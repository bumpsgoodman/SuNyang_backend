// 작성자: bumpsgoodman

#include "ServerInfo.h"
#include "Common/Assert.h"
#include "Common/ErrorCode/ErrorCode.h"
#include "Common/Logger/Logger.h"
#include "Common/Parser/INIParser.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

bool ServerInfo_Init(SERVER_INFO* pOutServerInfo, const char* pFilename)
{
    ASSERT(pOutServerInfo != NULL, "pOutServerInfo is NULL");
    ASSERT(pFilename != NULL, "pFilename is NULL");

    Logger_Print(LOG_LEVEL_INFO, "[ServerInfo] Start reading the server infor file.");

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

    Logger_Print(LOG_LEVEL_INFO, "[ServerInfo] End reading the server infor file with a success.\n"
                                 "[Server]\n"
                                 "HTTP port: %d\n"
                                 "HTTPS port: %d", pOutServerInfo->HttpPort, pOutServerInfo->HttpsPort);

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

void ServerInfo_Release(SERVER_INFO* pServerInfo)
{
    ASSERT(pServerInfo != NULL, "pServerInfo is NULL");
}