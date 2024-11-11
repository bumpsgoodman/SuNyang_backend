// 작성자: bumpsgoodman

#include "HttpRedirector.h"
#include "Common/Assert.h"
#include "Common/ErrorCode/ErrorCode.h"
#include "Common/PrimitiveType.h"
#include "Common/SafeDelete.h"
#include "Common/Logger/Logger.h"

#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#define MAX_HTTP_MESSAGE_SIZE 8192

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

    Logger_Print(LOG_LEVEL_INFO, "HttpRedirector - Start HTTP redirector.");

    if (pthread_create(&pHttpRedirector->Thread, NULL, redirectHttp, (void*)&pHttpRedirector->Port) < 0)
    {
        ErrorCode_SetLastError(ERROR_CODE_REDIRECTOR_FAILED_CREATE_THREAD);
        Logger_Print(LOG_LEVEL_ERROR, "HttpRedirector - End HTTP redirector with an error.\n"
                                      "Detail: %s\n"
                                      "%s", ErrorCode_GetLastErrorDetail(), strerror(errno));
        goto lb_return;
    }

lb_return:
    return;
}

void HttpRedirector_Shutdown(HTTP_REDIRECTOR* pHttpRedirector)
{
    ASSERT(pHttpRedirector != NULL, "pHttpRedirector is NULL");

    Logger_Print(LOG_LEVEL_INFO, "HttpRedirector - Shutdown HTTP redirector.");
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
        Logger_Print(LOG_LEVEL_ERROR, "HttpRedirector - End HTTP redirector with an error.\n"
                                      "Detail: %s\n"
                                      "%s", ErrorCode_GetLastErrorDetail(), strerror(errno));
        goto lb_return;
    }

    if (listen(httpSock, 5) < 0)
    {
        ErrorCode_SetLastError(ERROR_CODE_REDIRECTOR_FAILED_LISTEN_SOCKET);
        Logger_Print(LOG_LEVEL_ERROR, "HttpRedirector - End HTTP redirector with an error.\n"
                                      "Detail: %s\n"
                                      "%s", ErrorCode_GetLastErrorDetail(), strerror(errno));
        goto lb_return;
    }

    Logger_Print(LOG_LEVEL_INFO, "START SUNYANGI HTTP SERVER");

    const int httpEpoll = epoll_create1(0);
    if (httpEpoll < 0)
    {
        ErrorCode_SetLastError(ERROR_CODE_REDIRECTOR_FAILED_CREATE_EPOLL);
        Logger_Print(LOG_LEVEL_ERROR, "HttpRedirector - End HTTP redirector with an error.\n"
                                      "Detail: %s\n"
                                      "%s", ErrorCode_GetLastErrorDetail(), strerror(errno));
        goto lb_return;
    }
    
    while (true);

lb_return:
    Logger_Print(LOG_LEVEL_INFO, "HttpRedirector - Shutdown HTTP redirector with an error.");

    return NULL;
}