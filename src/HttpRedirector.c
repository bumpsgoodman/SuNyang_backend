// 작성자: bumpsgoodman

#include "HttpRedirector.h"
#include "Common/Assert.h"
#include "Common/PrimitiveType.h"
#include "Common/SafeDelete.h"
#include "Generic/ErrorCode/ErrorCode.h"
#include "Generic/Manager/ConfigManager.h"
#include "Generic/MemPool/StaticMemPool.h"
#include "Generic/Logger/Logger.h"
#include "Generic/Network/Network.h"

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

pthread_t HttpRedirector_Start(void)
{
    Logger_Print(LOG_LEVEL_INFO, "[HttpRedirector] Start HTTP redirector.");

    pthread_t httpRedirector;
    if (pthread_create(&httpRedirector, NULL, redirectHttp, NULL) != 0)
    {
        ErrorCode_SetLastError(ERROR_CODE_REDIRECTOR_FAILED_CREATE_THREAD);
        Logger_Print(LOG_LEVEL_ERROR, "[HttpRedirector] Shutdown HTTP redirector with an error.\n"
                                      "Detail: %s\n"
                                      "%s", ErrorCode_GetLastErrorDetail(), strerror(errno));
        return 0;
    }
    
    return httpRedirector;
}

static void* redirectHttp(void* pArg)
{
    IConfigManager* pConfigManager = GetConfigManager();
    const uint16_t httpPort = pConfigManager->GetHttpPort(pConfigManager);

    IStaticMemPool* pClientPool = NULL;
    IStaticMemPool* pRequestBufferPool = NULL;

    CreateStaticMemPool(&pClientPool);
    CreateStaticMemPool(&pRequestBufferPool);

    pClientPool->Init(pClientPool, 10, 1000, sizeof(CLIENT));
    pClientPool->Init(pRequestBufferPool, 10, 1000, MAX_HTTP_MESSAGE_SIZE);

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
                CLIENT* pClient = (CLIENT*)pClientPool->Alloc(pClientPool);

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
                char* pBuffer = (char*)pRequestBufferPool->Alloc(pRequestBufferPool);

                ssize_t bytesRead = read(pClient->Sock, pBuffer, MAX_HTTP_MESSAGE_SIZE - 1);

                // buffer overflow 발생은 해킹으로 간주
                if (bytesRead >= MAX_HTTP_MESSAGE_SIZE - 1)
                {
                    ErrorCode_SetLastError(ERROR_CODE_REDIRECTOR_BUFFER_OVERFLOW);
                    Logger_Print(LOG_LEVEL_WARNING, "[HttpRedirector] Buffer overflow.\n"
                                                    "Detail: %s", ErrorCode_GetLastErrorDetail());
                    goto lb_free_session;
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

            lb_free_session:
                close(pClient->Sock);
                pClientPool->Free(pClientPool, events[i].data.ptr);
                pClientPool->Free(pRequestBufferPool, pBuffer);
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

    SAFE_RELEASE(pClientPool);
    SAFE_RELEASE(pRequestBufferPool);

    DestroyStaticMemPool(pClientPool);
    DestroyStaticMemPool(pRequestBufferPool);

    return NULL;
}