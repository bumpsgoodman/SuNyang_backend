// 작성자: bumpsgoodman

#include "HttpRedirector.h"
#include "Common/Assert.h"
#include "Common/PrimitiveType.h"
#include "Common/ErrorCode/ErrorCode.h"
#include "Common/Logger/Logger.h"
#include "Common/Parser/INIParser.h"

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>

typedef struct SERVER_INFO
{
    uint16_t HttpPort;
    uint16_t HttpsPort;
} SERVER_INFO;

bool ParseServerInfo(SERVER_INFO* pOutServerInfo, const char* pFilename);
void ReleaseServerInfo(SERVER_INFO* pServerInfo);

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        Logger_Print(LOG_LEVEL_ERROR, "Unable to read the server info file.");
        goto lb_return;
    }

    bool bResult = false;
    SERVER_INFO serverInfo;
    HTTP_REDIRECTOR httpRedirector;

    // 초기화
    {
        Logger_Print(LOG_LEVEL_INFO, "[main] Start initializing SUNYANGI server.");

        bResult = ParseServerInfo(&serverInfo, argv[1]);
        if (!bResult)
        {
            goto lb_return;
        }

        bResult = HttpRedirector_Init(&httpRedirector, serverInfo.HttpPort);
        if (!bResult)
        {
            goto lb_return;
        }

        Logger_Print(LOG_LEVEL_INFO, "[main] End initializing SUNYANGI server successfully.");
    }

    // 실행
    {
        Logger_Print(LOG_LEVEL_INFO, "[main] Start SUNYANGI server.");

        HttpRedirector_Start(&httpRedirector);
    }

    pthread_join(httpRedirector.Thread, NULL);

    bResult = true;

lb_return:
    // 해제
    {
        ReleaseServerInfo(&serverInfo);
    }

    if (bResult)
    {
        Logger_Print(LOG_LEVEL_INFO, "[main] Shutdown SUNYANGI server successfully.");
    }
    else
    {
        Logger_Print(LOG_LEVEL_INFO, "[main] Shutdown SUNYANGI server with an error.");
    }

    return 0;
}

bool ParseServerInfo(SERVER_INFO* pOutServerInfo, const char* pFilename)
{
    ASSERT(pOutServerInfo != NULL, "pOutServerInfo is NULL");
    ASSERT(pFilename != NULL, "pFilename is NULL");

    Logger_Print(LOG_LEVEL_INFO, "[ServerInfo] Start reading the server information file.");

    bool bResult = false;

    const size_t filenameLength = strlen(pFilename);
    if (filenameLength < 4 || strcasecmp(&pFilename[filenameLength - 4], ".ini") != 0)
    {
        ErrorCode_SetLastError(ERROR_CODE_SERVER_INFO_INVALID_FILE_EXTENSION);
        goto lb_return;
    }

    INI_PARSER parser;
    if (!INIParser_Init(&parser))
    {
        ErrorCode_SetLastError(ERROR_CODE_SERVER_INFO_FAILED_INIT_INI_PARSER);
        goto lb_return;
    }

    if (!INIParser_Parse(&parser, pFilename))
    {
        ErrorCode_SetLastError(ERROR_CODE_SERVER_INFO_FAILED_PARSE_INI_FILE);
        goto lb_release;
    }

    if (!INIParser_GetValueShort(&parser, "Server", "httpPort", (short*)&pOutServerInfo->HttpPort))
    {
        ErrorCode_SetLastError(ERROR_CODE_SERVER_INFO_FAILED_GET_HTTP_PORT);
        goto lb_release;
    }

    if (!INIParser_GetValueShort(&parser, "Server", "httpsPort", (short*)&pOutServerInfo->HttpsPort))
    {
        ErrorCode_SetLastError(ERROR_CODE_SERVER_INFO_FAILED_GET_HTTPS_PORT);
        goto lb_release;
    }

    Logger_Print(LOG_LEVEL_INFO, "[ServerInfo] End reading the server information file successfully.\n"
                                 "------------------\n"
                                 "[Server]\n"
                                 "HTTP port: %d\n"
                                 "HTTPS port: %d\n"
                                 "------------------", pOutServerInfo->HttpPort, pOutServerInfo->HttpsPort);

    bResult = true;

lb_release:
    INIParser_Release(&parser);

lb_return:
    if (!bResult)
    {
        Logger_Print(LOG_LEVEL_ERROR, "[ServerInfo] End reading the server info file with an error.\n"
                                      "Detail: %s", ErrorCode_GetLastErrorDetail());
    }

    return bResult;
}

void ReleaseServerInfo(SERVER_INFO* pServerInfo)
{
    ASSERT(pServerInfo != NULL, "pServerInfo is NULL");


}