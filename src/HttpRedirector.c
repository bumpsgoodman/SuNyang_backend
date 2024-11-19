// 작성자: bumpsgoodman

#include "HttpRedirector.h"
#include "Common/Assert.h"
#include "Common/ErrorCode/ErrorCode.h"
#include "Common/PrimitiveType.h"
#include "Common/SafeDelete.h"
#include "Common/MemPool/StaticMemPool.h"
#include "Common/Logger/Logger.h"
#include "Common/Network/Network.h"

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#define MAX_HTTP_MESSAGE_SIZE 8192
#define MAX_EPOLL_EVENTS 128

typedef struct CLIENT
{
    int Sock;
    struct sockaddr_in Addr;
} CLIENT;

static void* redirectHttp(void* pArg);

uint16_t g_httpPort;

pthread_t HttpRedirector_Start(const uint16_t httpPort)
{
    Logger_Print(LOG_LEVEL_INFO, "[HttpRedirector] Start HTTP redirector.");

    g_httpPort = httpPort;

    pthread_t httpRedirector;
    if (pthread_create(&httpRedirector, NULL, redirectHttp, (void*)&g_httpPort) != 0)
    {
        ErrorCode_SetLastError(ERROR_CODE_REDIRECTOR_FAILED_CREATE_THREAD);
        Logger_Print(LOG_LEVEL_ERROR, "[HttpRedirector] Shutdown HTTP redirector with an error.\n"
                                      "Detail: %s\n"
                                      "%s", ErrorCode_GetLastErrorDetail(), strerror(errno));
        return 0;
    }

lb_return:
    return httpRedirector;
}

static void* redirectHttp(void* pArg)
{
    const uint16_t httpPort = *(uint16_t*)pArg;
    STATIC_MEM_POOL clientPool;
    STATIC_MEM_POOL requestBufferPool;

    StaticMemPool_Init(&clientPool, 10, 1000, sizeof(CLIENT));
    StaticMemPool_Init(&requestBufferPool, 10, 1000, MAX_HTTP_MESSAGE_SIZE);

    // HTTP 소켓 생성
    const int httpSock = socket(AF_INET, SOCK_STREAM, 0);
    if (httpSock < 0)
    {
        ErrorCode_SetLastError(ERROR_CODE_REDIRECTOR_FAILED_CREATE_SOCKET);
        Logger_Print(LOG_LEVEL_ERROR, "Failed to create http socket.\n"
                                      "Detail: %s", strerror(errno));
        goto lb_return;
    }

    // HTTP 소켓 바인딩
    struct sockaddr_in addr = { 0, };
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(httpPort);
    if (bind(httpSock, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        ErrorCode_SetLastError(ERROR_CODE_REDIRECTOR_FAILED_BIND_SOCKET);
        Logger_Print(LOG_LEVEL_ERROR, "[HttpRedirector] Shutdown HTTP redirector with an error.\n"
                                      "Detail: %s\n"
                                      "%s", ErrorCode_GetLastErrorDetail(), strerror(errno));
        goto lb_return;
    }

    // HTTP 소켓 리스닝
    if (listen(httpSock, 5) < 0)
    {
        ErrorCode_SetLastError(ERROR_CODE_REDIRECTOR_FAILED_LISTEN_SOCKET);
        Logger_Print(LOG_LEVEL_ERROR, "[HttpRedirector] Shutdown HTTP redirector with an error.\n"
                                      "Detail: %s\n"
                                      "%s", ErrorCode_GetLastErrorDetail(), strerror(errno));
        goto lb_return;
    }

    // HTTP epoll 생성
    const int httpEpoll = epoll_create1(0);
    if (httpEpoll < 0)
    {
        ErrorCode_SetLastError(ERROR_CODE_REDIRECTOR_FAILED_CREATE_EPOLL);
        Logger_Print(LOG_LEVEL_ERROR, "[HttpRedirector] Shutdown HTTP redirector with an error.\n"
                                      "Detail: %s\n"
                                      "%s", ErrorCode_GetLastErrorDetail(), strerror(errno));
        goto lb_return;
    }

    // HTTP epoll 등록
    struct epoll_event event = { 0, };
    event.events = EPOLLIN | EPOLLOUT | EPOLLERR;
    event.data.fd = httpSock;
    if (epoll_ctl(httpEpoll, EPOLL_CTL_ADD, httpSock, &event) == -1)
    {
        ErrorCode_SetLastError(ERROR_CODE_REDIRECTOR_FAILED_WATCH_EPOLL_DESCRIPTER);
        Logger_Print(LOG_LEVEL_ERROR, "[HttpRedirector] Failed to watch epoll descriptor.\n"
                                      "Detail: %s\n",
                                      "%s", ErrorCode_GetLastErrorDetail(), strerror(errno));
        goto lb_return;
    }

    struct epoll_event events[MAX_EPOLL_EVENTS];
    while (true)
    {
        const int numEvents = epoll_wait(httpEpoll, events, MAX_EPOLL_EVENTS, 0);
        for (size_t i = 0; i < numEvents; ++i)
        {
            if (events[i].data.fd == httpSock)
            {
                CLIENT* pClient = (CLIENT*)StaticMemPool_Alloc(&clientPool);

                socklen_t clientAddrLen = sizeof(struct sockaddr_in);
                const int clientSock = accept(httpSock, (struct sockaddr*)&pClient->Addr, &clientAddrLen);
                if (clientSock < 0)
                {
                    ErrorCode_SetLastError(ERROR_CODE_REDIRECTOR_FAILED_CREATE_SOCKET);
                    Logger_Print(LOG_LEVEL_ERROR, "[HttpRedirector] Failed to create http client socket.\n"
                                                  "Detail: %s\n"
                                                  "%s", ErrorCode_GetLastErrorDetail(), strerror(errno));
                    continue;
                }

                pClient->Sock = clientSock;

                // client socket epoll 등록
                struct epoll_event event;
                event.events = EPOLLIN | EPOLLET;
                event.data.ptr = pClient;
                if (epoll_ctl(httpEpoll, EPOLL_CTL_ADD, clientSock, &event) == -1)
                {
                    ErrorCode_SetLastError(ERROR_CODE_REDIRECTOR_FAILED_WATCH_EPOLL_DESCRIPTER);
                    Logger_Print(LOG_LEVEL_ERROR, "[HttpRedirector] Failed to watch epoll descriptor.\n"
                                                  "Detail: %s\n",
                                                  "%s", ErrorCode_GetLastErrorDetail(), strerror(errno));
                    close(clientSock);
                    continue;
                }
            }
            else if (events[i].events & EPOLLIN)
            {
                CLIENT* pClient = (CLIENT*)events[i].data.ptr;
                char* pBuffer = (char*)StaticMemPool_Alloc(&requestBufferPool);

                ssize_t bytesRead = read(pClient->Sock, pBuffer, MAX_HTTP_MESSAGE_SIZE - 1);
                printf("\n@@@@@@@@@@@@@@@@\n");
                printf("%s", pBuffer);
                printf("@@@@@@@@@@@@@@@@\n");

                // buffer overflow 발생은 해킹으로 간주
                if (bytesRead >= MAX_HTTP_MESSAGE_SIZE - 1)
                {
                    ErrorCode_SetLastError(ERROR_CODE_REDIRECTOR_BUFFER_OVERFLOW);
                    Logger_Print(LOG_LEVEL_WARNING, "[HttpRedirector] Buffer overflow.\n"
                                                    "Detail: %s", ErrorCode_GetLastErrorDetail());
                    close(pClient->Sock);
                    continue;
                }

                if (bytesRead > 0)
                {
                    pBuffer[bytesRead] = '\0';
                    const char* pMethod = strtok(pBuffer, " ");
                    const char* pLocation = strtok(NULL, " ");
                    const char* pVersion = strtok(NULL, " \r\n");

                    char ipv4[IPV4_LENGTH];
                    const struct sockaddr_in* pAddr = &pClient->Addr;
                    Network_Ipv4ToString(pAddr->sin_addr.s_addr, BYTE_ORDERING_LITTLE_ENDIAN, ipv4, BYTE_ORDERING_BIG_ENDIAN);
                    Logger_Print(LOG_LEVEL_INFO, "[HttpRedirector] Request - %s %s %s, %s:%d", pMethod, pLocation, pVersion, ipv4, pAddr->sin_port);

                    if (strcmp(pMethod, "GET") == 0)
                    {
                        char response[256];
                        sprintf(response, "HTTP/1.1 301 Moved Permanently\r\n"
                                          "Locatoin: https://localhost:8081%s\r\n"
                                          "\r\n", pLocation);
                        Logger_Print(LOG_LEVEL_INFO, "[HttpRedirector] 301 Moved Permanently.\n"
                                                     "Detail: %s", pLocation);
                        write(pClient->Sock, response, strlen(response));
                    }
                    
                }

                close(pClient->Sock);
                StaticMemPool_Free(&clientPool, events[i].data.ptr);
                StaticMemPool_Free(&requestBufferPool, pBuffer);
            }
        }
    }

    Logger_Print(LOG_LEVEL_INFO, "[HttpRedirector] Shutdown HTTP redirector successfully.");

lb_return:
    if (httpEpoll >= 0)
    {
        close(httpEpoll);
    }

    if (httpSock >= 0)
    {
        close(httpSock);
    }

    StaticMemPool_Release(&clientPool);

    return NULL;
}