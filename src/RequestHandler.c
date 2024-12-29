// 작성자: bumpsgoodman

#include "RequestHandler.h"
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
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#define MAX_HTTP_MESSAGE_SIZE 8192
#define MAX_EPOLL_EVENTS 128

typedef struct CLIENT
{
    int Sock;
    struct sockaddr_in Addr;
    SSL* pSsl;
} CLIENT;

static void* interpretHttp(void* pArg);

pthread_t RequestHandler_Start(void)
{
    Logger_Print(LOG_LEVEL_INFO, "[HttpInterpreter] Start HTTP interpreter.");

    pthread_t httpInterPreter;
    if (pthread_create(&httpInterPreter, NULL, interpretHttp, NULL) != 0)
    {
        ErrorCode_SetLastError(ERROR_CODE_REQUEST_HANDLER_FAILED_CREATE_THREAD);
        Logger_Print(LOG_LEVEL_ERROR, "[HttpInterpreter] Shutdown HTTP interpreter with an error.\n"
                                      "Detail: %s\n"
                                      "%s", ErrorCode_GetLastErrorDetail(), strerror(errno));
        return 0;
    }
    
    return httpInterPreter;
}

static void* interpretHttp(void* pArg)
{
    IConfigManager* pConfigManager = GetConfigManager();
    const uint16_t httpsPort = pConfigManager->GetHttpsPort(pConfigManager);

    IStaticMemPool* pClientPool = NULL;
    IStaticMemPool* pRequestBufferPool = NULL;

    CreateStaticMemPool(&pClientPool);
    CreateStaticMemPool(&pRequestBufferPool);

    // SSL CTX 생성
    SSL_CTX* pCtx = SSL_CTX_new(TLS_server_method());
    SSL_CTX_set_options(pCtx, SSL_OP_ALL);

    pClientPool->Init(pClientPool, 10, 1000, sizeof(CLIENT));
    pClientPool->Init(pRequestBufferPool, 10, 1000, MAX_HTTP_MESSAGE_SIZE);

    // HTTPS 소켓 생성
    const int httpsSock = socket(AF_INET, SOCK_STREAM, 0);
    if (httpsSock < 0)
    {
        ErrorCode_SetLastError(ERROR_CODE_REQUEST_HANDLER_FAILED_CREATE_SOCKET);
        Logger_Print(LOG_LEVEL_ERROR, "Failed to create https socket.\n"
                                      "Detail: %s", strerror(errno));
        goto lb_return;
    }

    // HTTPS 소켓 바인딩
    struct sockaddr_in addr = { 0, };
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(httpsPort);
    if (bind(httpsSock, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        ErrorCode_SetLastError(ERROR_CODE_REQUEST_HANDLER_FAILED_BIND_SOCKET);
        Logger_Print(LOG_LEVEL_ERROR, "[HttpInterpreter] Shutdown HTTP interpreter with an error.\n"
                                      "Detail: %s\n"
                                      "%s", ErrorCode_GetLastErrorDetail(), strerror(errno));
        goto lb_return;
    }

    // HTTPS 소켓 리스닝
    if (listen(httpsSock, 5) < 0)
    {
        ErrorCode_SetLastError(ERROR_CODE_REQUEST_HANDLER_FAILED_LISTEN_SOCKET);
        Logger_Print(LOG_LEVEL_ERROR, "[HttpInterpreter] Shutdown HTTP interpreter with an error.\n"
                                      "Detail: %s\n"
                                      "%s", ErrorCode_GetLastErrorDetail(), strerror(errno));
        goto lb_return;
    }

    // HTTPS epoll 생성
    const int httpEpoll = epoll_create1(0);
    if (httpEpoll < 0)
    {
        ErrorCode_SetLastError(ERROR_CODE_REQUEST_HANDLER_FAILED_CREATE_EPOLL);
        Logger_Print(LOG_LEVEL_ERROR, "[HttpInterpreter] Shutdown HTTP interpreter with an error.\n"
                                      "Detail: %s\n"
                                      "%s", ErrorCode_GetLastErrorDetail(), strerror(errno));
        goto lb_return;
    }

    // HTTPS epoll 등록
    struct epoll_event event = { 0, };
    event.events = EPOLLIN | EPOLLOUT | EPOLLERR;
    event.data.fd = httpsSock;
    if (epoll_ctl(httpEpoll, EPOLL_CTL_ADD, httpsSock, &event) == -1)
    {
        ErrorCode_SetLastError(ERROR_CODE_REQUEST_HANDLER_FAILED_WATCH_EPOLL_DESCRIPTER);
        Logger_Print(LOG_LEVEL_ERROR, "[HttpInterpreter] Failed to watch epoll descriptor.\n"
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
            if (events[i].data.fd == httpsSock)
            {
                CLIENT* pClient = (CLIENT*)pClientPool->Alloc(pClientPool);

                socklen_t clientAddrLen = sizeof(struct sockaddr_in);
                const int clientSock = accept(httpsSock, (struct sockaddr*)&pClient->Addr, &clientAddrLen);
                if (clientSock < 0)
                {
                    ErrorCode_SetLastError(ERROR_CODE_REQUEST_HANDLER_FAILED_CREATE_SOCKET);
                    Logger_Print(LOG_LEVEL_ERROR, "[HttpInterpreter] Failed to create http client socket.\n"
                                                  "Detail: %s\n"
                                                  "%s", ErrorCode_GetLastErrorDetail(), strerror(errno));
                    continue;
                }

                pClient->Sock = clientSock;

                // SSL 생성
                SSL* pSsl = SSL_new(pCtx);
	            SSL_set_fd(pSsl, pClient->Sock);
                if (SSL_use_certificate_chain_file(pSsl, pConfigManager->GetCertPath(pConfigManager)) <= 0)
                {
                    ERR_print_errors_fp(stderr);
                    close(clientSock);
                    continue;
                }

                if (SSL_use_PrivateKey_file(pSsl, pConfigManager->GetPrivateKeyPath(pConfigManager), SSL_FILETYPE_PEM) <= 0)
                {
                    ERR_print_errors_fp(stderr);
                    close(clientSock);
                    continue;
                }
	            SSL_accept(pSsl);

                pClient->pSsl = pSsl;

                // client socket epoll 등록
                struct epoll_event event;
                event.events = EPOLLIN | EPOLLET;
                event.data.ptr = pClient;
                if (epoll_ctl(httpEpoll, EPOLL_CTL_ADD, clientSock, &event) == -1)
                {
                    ErrorCode_SetLastError(ERROR_CODE_REQUEST_HANDLER_FAILED_WATCH_EPOLL_DESCRIPTER);
                    Logger_Print(LOG_LEVEL_ERROR, "[HttpInterpreter] Failed to watch epoll descriptor.\n"
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

                const int bytesRead = SSL_read(pClient->pSsl, pBuffer, MAX_HTTP_MESSAGE_SIZE - 1);

                // buffer overflow 발생은 해킹으로 간주
                if (bytesRead >= MAX_HTTP_MESSAGE_SIZE - 1)
                {
                    ErrorCode_SetLastError(ERROR_CODE_REQUEST_HANDLER_BUFFER_OVERFLOW);
                    Logger_Print(LOG_LEVEL_WARNING, "[HttpInterpreter] Buffer overflow.\n"
                                                    "Detail: %s", ErrorCode_GetLastErrorDetail());
                    goto lb_free_session;
                }

                if (bytesRead <= 0)
                {
                    goto lb_free_session;
                }

                pBuffer[bytesRead] = '\0';
                const char* pMethod = strtok(pBuffer, " ");
                const char* pLocation = strtok(NULL, " ");
                const char* pVersion = strtok(NULL, " \r\n");

                char ipv4[IPV4_LENGTH];
                const struct sockaddr_in* pAddr = &pClient->Addr;
                Network_Ipv4ToString(pAddr->sin_addr.s_addr, BYTE_ORDERING_LITTLE_ENDIAN, ipv4, BYTE_ORDERING_BIG_ENDIAN);
                Logger_Print(LOG_LEVEL_INFO, "[HttpInterpreter] Request - %s %s %s, %s:%d", pMethod, pLocation, pVersion, ipv4, pAddr->sin_port);

                // TODO: 요청 메시지 분석하기

                if (strcmp(pMethod, "GET") == 0)
                {
                    char response[256];
                    sprintf(response, "HTTP/1.1 301 Moved Permanently\r\n"
                        "Locatoin: https://localhost:8081%s\r\n"
                        "\r\n", pLocation);
                    Logger_Print(LOG_LEVEL_INFO, "[HttpInterpreter] 301 Moved Permanently.\n"
                        "Detail: %s", pLocation);
                    SSL_write(pClient->pSsl, response, strlen(response));
                }

            lb_free_session:
                SSL_shutdown(pClient->pSsl);
                SSL_free(pClient->pSsl);
                close(pClient->Sock);
                pClientPool->Free(pClientPool, events[i].data.ptr);
                pClientPool->Free(pRequestBufferPool, pBuffer);
            }
        }
    }

    Logger_Print(LOG_LEVEL_INFO, "[HttpInterpreter] Shutdown HTTP interpreter successfully.");

lb_return:
    if (httpEpoll >= 0)
    {
        close(httpEpoll);
    }

    if (httpsSock >= 0)
    {
        close(httpsSock);
    }

    if (pCtx != NULL)
    {
        SSL_CTX_free(pCtx);
    }

    SAFE_RELEASE(pClientPool);
    SAFE_RELEASE(pRequestBufferPool);

    DestroyStaticMemPool(pClientPool);
    DestroyStaticMemPool(pRequestBufferPool);

    return NULL;
}