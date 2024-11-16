// 작성자: bumpsgoodman

#include "HttpRedirector.h"
#include "Common/Assert.h"
#include "Common/ErrorCode/ErrorCode.h"
#include "Common/PrimitiveType.h"
#include "Common/SafeDelete.h"
#include "Common/Logger/Logger.h"

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#define MAX_HTTP_MESSAGE_SIZE 8192
#define MAX_EPOLL_EVENTS 128

static void* redirectHttp(void* pPort);

bool HttpRedirector_Init(HTTP_REDIRECTOR* pHttpRedirector, uint16_t port)
{
    ASSERT(pHttpRedirector != NULL, "pHttpRedirector is NULL");

    pHttpRedirector->Port = port;
    return true;
}

void HttpRedirector_Start(HTTP_REDIRECTOR* pHttpRedirector)
{
    ASSERT(pHttpRedirector != NULL, "pHttpRedirector is NULL");

    Logger_Print(LOG_LEVEL_INFO, "[HttpRedirector] Start HTTP redirector.");

    if (pthread_create(&pHttpRedirector->Thread, NULL, redirectHttp, (void*)&pHttpRedirector->Port) < 0)
    {
        ErrorCode_SetLastError(ERROR_CODE_REDIRECTOR_FAILED_CREATE_THREAD);
        Logger_Print(LOG_LEVEL_ERROR, "[HttpRedirector] Shutdown HTTP redirector with an error.\n"
                                      "Detail: %s\n"
                                      "%s", ErrorCode_GetLastErrorDetail(), strerror(errno));
        goto lb_return;
    }

lb_return:
    return;
}

static void* redirectHttp(void* pPort)
{
    const uint16_t httpPort = *(uint16_t*)pPort;

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
    struct epoll_event event;
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
                struct sockaddr_in clientAddr;
                socklen_t clientAddrLen = sizeof(clientAddr);
                const int clientSock = accept(httpSock, (struct sockaddr*)&clientAddr, &clientAddrLen);
                if (clientSock < 0)
                {
                    ErrorCode_SetLastError(ERROR_CODE_REDIRECTOR_FAILED_CREATE_SOCKET);
                    Logger_Print(LOG_LEVEL_ERROR, "[HttpRedirector] Failed to create http client socket.\n"
                                                "Detail: %s\n"
                                                "%s", ErrorCode_GetLastErrorDetail(), strerror(errno));
                    continue;
                }

                // client socket epoll 등록
                struct epoll_event event;
                event.events = EPOLLIN | EPOLLOUT | EPOLLERR;
                event.data.fd = clientSock;
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
            else
            {
                char buffer[MAX_HTTP_MESSAGE_SIZE];
                ssize_t bytesRead = read(events[i].data.fd, buffer, MAX_HTTP_MESSAGE_SIZE - 1);
                printf("\n@@@@@@@@@@@@@@@@\n");
                printf("%s\n", buffer);
                printf("@@@@@@@@@@@@@@@@\n");
                if (bytesRead >= MAX_HTTP_MESSAGE_SIZE - 1)
                {
                    ErrorCode_SetLastError(ERROR_CODE_REDIRECTOR_BUFFER_OVERFLOW);
                    Logger_Print(LOG_LEVEL_WARNING, "[HttpRedirector] Buffer overflow.\n"
                                                    "Detail: %s", ErrorCode_GetLastErrorDetail());

                    while ((bytesRead = read(events[i].data.fd, buffer, MAX_HTTP_MESSAGE_SIZE - 1)) > 0);

                    continue;
                }

                if (bytesRead > 0)
                {
                    buffer[bytesRead] = '\0';
                    const char* pMethod = strtok(buffer, " ");
                    const char* pLocation = strtok(NULL, " ");

                    char response[256];


                    const char* pResponse = "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/plain\r\n"
                    "Content-Length: 11\r\n"
                    "\r\n"
                    "Hello world\r\n";
                    write(events[i].data.fd, pResponse, strlen(pResponse));
                }

                close(events[i].data.fd);
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

    return NULL;
}