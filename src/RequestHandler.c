// 작성자: bumpsgoodman

#include "RequestHandler.h"
#include "Common/Assert.h"
#include "Common/SafeDelete.h"
#include "Common/Util/HashFunctions.h"
#include "Generic/Container/FixedArray.h"
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
#define MAX_CHILDREN 4

#define GET_METHOD_HASH32 0x548c
#define POST_METHOD_HASH32 0x54534f50
#define HEAD_METHOD_HASH32 0x44414548
#define PUT_METHOD_HASH32 0x54a5
#define DELETE_METHOD_HASH32 0x45548a90
#define CONNECT_METHOD_HASH32 0x544393e0
#define OPTIONS_METHOD_HASH32 0x534ea3e8
#define TRACE_METHOD_HASH32 0x454341a6
#define PATCH_METHOD_HASH32 0x48435491

typedef struct CLIENT
{
    int Sock;
    struct sockaddr_in Addr;
    SSL* pSsl;
} CLIENT;

typedef struct PATH_NODE
{
    uint32_t Id;
    bool bHasParam;
    IFixedArray* Children;
} PATH_NODE;

PATH_NODE* g_PathRoot;

static void* handleRequest(void* pArg);
static bool interpretRequest(char* pRequestMessage, const size_t requestMessageLength, REQUEST* pOutRequest);

static bool insertPathNode(PATH_NODE* pRoot, PATH_NODE* pPathNode);

pthread_t RequestHandler_Start(void)
{
    Logger_Print(LOG_LEVEL_INFO, "[RequestHandler] Start request handler.");

    pthread_t httpInterPreter;
    if (pthread_create(&httpInterPreter, NULL, handleRequest, NULL) != 0)
    {
        ErrorCode_SetLastError(ERROR_CODE_REQUEST_HANDLER_FAILED_CREATE_THREAD);
        Logger_Print(LOG_LEVEL_ERROR, "[RequestHandler] Shutdown request handler with an error.\n"
                                      "Detail: %s\n"
                                      "%s", ErrorCode_GetLastErrorDetail(), strerror(errno));
        return 0;
    }
    
    return httpInterPreter;
}

static void* handleRequest(void* pArg)
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
        Logger_Print(LOG_LEVEL_ERROR, "[RequestHandler] Shutdown request handler with an error.\n"
                                      "Detail: %s\n"
                                      "%s", ErrorCode_GetLastErrorDetail(), strerror(errno));
        goto lb_return;
    }

    // HTTPS 소켓 리스닝
    if (listen(httpsSock, 5) < 0)
    {
        ErrorCode_SetLastError(ERROR_CODE_REQUEST_HANDLER_FAILED_LISTEN_SOCKET);
        Logger_Print(LOG_LEVEL_ERROR, "[RequestHandler] Shutdown request handler with an error.\n"
                                      "Detail: %s\n"
                                      "%s", ErrorCode_GetLastErrorDetail(), strerror(errno));
        goto lb_return;
    }

    // HTTPS epoll 생성
    const int httpEpoll = epoll_create1(0);
    if (httpEpoll < 0)
    {
        ErrorCode_SetLastError(ERROR_CODE_REQUEST_HANDLER_FAILED_CREATE_EPOLL);
        Logger_Print(LOG_LEVEL_ERROR, "[RequestHandler] Shutdown request handler with an error.\n"
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
        Logger_Print(LOG_LEVEL_ERROR, "[RequestHandler] Failed to watch epoll descriptor.\n"
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
                    Logger_Print(LOG_LEVEL_ERROR, "[RequestHandler] Failed to create http client socket.\n"
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
                    Logger_Print(LOG_LEVEL_ERROR, "[RequestHandler] Failed to watch epoll descriptor.\n"
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
                    Logger_Print(LOG_LEVEL_WARNING, "[RequestHandler] Buffer overflow.\n"
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
                Logger_Print(LOG_LEVEL_INFO, "[RequestHandler] Request - %s %s %s, %s:%d", pMethod, pLocation, pVersion, ipv4, pAddr->sin_port);

                REQUEST request;
                if (!interpretRequest(pBuffer, bytesRead, &request))
                {
                    goto lb_free_session;
                }

                // TODO: 요청 처리

                if (strcmp(pMethod, "GET") == 0)
                {
                    char response[256];
                    sprintf(response, "HTTP/1.1 301 Moved Permanently\r\n"
                        "Locatoin: https://localhost:8081%s\r\n"
                        "\r\n", pLocation);
                    Logger_Print(LOG_LEVEL_INFO, "[RequestHandler] 301 Moved Permanently.\n"
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

    Logger_Print(LOG_LEVEL_INFO, "[RequestHandler] Shutdown request handler successfully.");

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

bool RequestHandler_RegisterPath(const HTTP_METHOD method, const char* pPath, const size_t pathLength)
{
    ASSERT(method < HTTP_METHOD_NOT_SUPPORTED, "Invalid method");
    ASSERT(pPath != NULL, "pPath is NULL");
    ASSERT(pathLength > 0, "pathLength is 0");

    if (*pPath != '/')
    {
        return false;
    }

    const char* pStart = pPath + 1;
    const char* pEnd = pStart;
    while (*pEnd != '\0')
    {
        if (*pEnd == '/')
        {
            const size_t len = pEnd - pStart + 1;
            const uint32_t hash32 = SuNyangiHash32(pStart, len);

            PATH_NODE* pNode = (PATH_NODE*)malloc(sizeof(PATH_NODE));
            ASSERT(pNode != NULL, "Failed to malloc");

            pNode->Id = hash32;
            pNode->bHasParam = false;
            CreateFixedArray(&pNode->Children);
            pNode->Children->Init(pNode->Children, MAX_CHILDREN, sizeof(PATH_NODE));

            if (*(pEnd + 1) == '*')
            {
                pNode->bHasParam = true;

                pEnd += 3;
                pStart = pEnd;
            }

            // TODO: Insert Node

            continue;
        }

        ++pEnd;
    }

    return true;
}

static bool interpretRequest(char* pRequestMessage, const size_t requestMessageLength, REQUEST* pOutRequest)
{
    ASSERT(pRequestMessage != NULL, "pRequestMessage is NULL");
    ASSERT(requestMessageLength > 0, "Invalid requestMessageLength");
    ASSERT(pOutRequest != NULL, "pOutRequest is NULL");

    static const char* DELIM = " \r\n";

    bool bResult = false;

    // method 파싱
    {
        char* pMethod = strtok(pRequestMessage, DELIM);
        if (pMethod == NULL)
        {
            ErrorCode_SetLastError(ERROR_CODE_REQUEST_HANDLER_FAILED_PARSE_METHOD);
            goto lb_return;
        }

        const size_t methodLength = strlen(pMethod);
        const uint32_t methodHash32 = SuNyangiHash32(pMethod, methodLength);
        switch (methodHash32)
        {
        case GET_METHOD_HASH32:
            pOutRequest->Method = HTTP_METHOD_GET;
            break;
        case POST_METHOD_HASH32:
            pOutRequest->Method = HTTP_METHOD_POST;
            break;
        case HEAD_METHOD_HASH32:
            pOutRequest->Method = HTTP_METHOD_HEAD;
            break;
        case PUT_METHOD_HASH32:
            pOutRequest->Method = HTTP_METHOD_PUT;
            break;
        case DELETE_METHOD_HASH32:
            pOutRequest->Method = HTTP_METHOD_DELETE;
            break;
        case OPTIONS_METHOD_HASH32:
            pOutRequest->Method = HTTP_METHOD_OPTIONS;
        case CONNECT_METHOD_HASH32: // fall-through
        case TRACE_METHOD_HASH32:   // fall-through
        case PATCH_METHOD_HASH32:   // fall-through
        default:
            ErrorCode_SetLastError(ERROR_CODE_REQUEST_HANDLER_NOT_SUPPORTED_METHOD);
            goto lb_return;
        }
    }

    // path 파싱
    {
        char* pRequestPath = strtok(NULL, DELIM);
        if (pRequestPath == NULL)
        {
            goto lb_return;
        }

        pOutRequest->pPath = pRequestPath;
    }

    // version 파싱
    {
        char* pHttpVersion = strtok(NULL, DELIM);
        if (pHttpVersion == NULL)
        {
            goto lb_return;
        }

        pOutRequest->Version.Major = atoi(pHttpVersion + 5);
        pOutRequest->Version.Minor = atoi(pHttpVersion + 7);
    }

    bResult = true;

lb_return:
    return bResult;
}