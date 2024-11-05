// 작성자: bumpsgoodman

#include "HttpRedirector.h"
#include "../Common/Assert.h"
#include "../Common/ErrorHandler.h"
#include "../Common/INIParser.h"
#include "../Common/PrimitiveType.h"
#include "../Common/SafeDelete.h"
#include "HttpMessage.h"
#include "../Logger/Logger.h"

#include <ctype.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_HTTP_MESSAGE_SIZE 8192

void* RedirectHttp(void* pArg)
{
    Logger_Print(LOG_LEVEL_INFO, "HTTP - START");

    uint16_t port = *(uint16_t*)pArg;

    // http 소켓 생성
    int httpSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (httpSock < 0)
    {
        Logger_Print(LOG_LEVEL_ERROR, "HTTP - Failed to create HTTP socket");
        goto lb_return;
    }

    int opt = 1;
    if (setsockopt(httpSock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) 
    {
        Logger_Print(LOG_LEVEL_ERROR, "HTTP - Failed to set socket options");
        goto lb_return;
    }

    // 소켓 바인딩
    struct sockaddr_in serverAddr = { 0, };
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);
    if (bind(httpSock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
    {
        Logger_Print(LOG_LEVEL_ERROR, "HTTP - Failed to bind HTTP socket.");
        goto lb_return;
    }

    listen(httpSock, 10);

    // 클라이언트 소켓 생성
    struct sockaddr_in clientAddr = { 0, };
    socklen_t clientLen = sizeof(clientAddr);
    int clientSock;

    while (true)
    {
        printf("HTTP - accept\n");
        clientSock = accept(httpSock, (struct sockaddr*)&clientAddr, &clientLen);
        if (clientSock < 0)
        {
            Logger_Print(LOG_LEVEL_ERROR, "HTTP - Failed to create HTTP client socket.");
            goto lb_release_request;
        }
        printf("HTTP - accept2\n");

        char* pBuffer = (char*)malloc(MAX_HTTP_MESSAGE_SIZE);

        int n = read(clientSock, pBuffer, MAX_HTTP_MESSAGE_SIZE - 1);
        if (n < 0)
        {
            goto lb_release_request;
        }

        START_LINE startLine;
        if (!ParseStartLine(pBuffer, &startLine))
        {
            Logger_Print(LOG_LEVEL_ERROR, "HTTP - Failed to parse start line.");
            goto lb_release_request;
        }

        char response[1024];
        sprintf(response, "HTTP/%d.%d 302 %s\r\n"
                            "Location: https://sunyangi.com%s\r\n\r\n",
                            startLine.Version.Major,
                            startLine.Version.Minor,
                            GetHttpStatusString(HTTP_STATUS_FOUND),
                            startLine.pRequestTarget);
        Logger_Print(LOG_LEVEL_INFO, response);
        write(clientSock, response, strlen(response));

    lb_release_request:
        SAFE_FREE(pBuffer);
        if (clientSock >= 0)
        {
            close(clientSock);
        }
    }

lb_return:
    if (httpSock >= 0)
    {
        close(httpSock);
    }

    return NULL;
}
