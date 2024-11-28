// 작성자: bumpsgoodman

#include "Common/Assert.h"
#include "Common/SafeDelete.h"
#include "Generic/ErrorCode/ErrorCode.h"
#include "Generic/Logger/Logger.h"
#include "Generic/Manager/Parser/INIParser.h"
#include "Generic/Manager/ConfigManager.h"

#include <stdlib.h>
#include <string.h>
#include <strings.h>

typedef struct CONFIG_MANAGER
{
    IConfigManager vtbl;

    uint16_t HttpPort;
    uint16_t HttpsPort;
} CONFIG_MANAGER;

static bool Init(IConfigManager* pThis, const char* pConfigFilePath);
static void Release(IConfigManager* pThis);

static uint16_t GetHttpPort(const IConfigManager* pThis);
static uint16_t GetHttpsPort(const IConfigManager* pThis);

static const IConfigManager s_vtbl =
{
    Init,
    Release,

    GetHttpPort,
    GetHttpsPort,
};

static bool Init(IConfigManager* pThis, const char* pConfigFilePath)
{
    ASSERT(pThis != NULL, "pThis is NULL");
    ASSERT(pConfigFilePath != NULL, "pConfigFilePath is NULL");

    Logger_Print(LOG_LEVEL_INFO, "[ConfigManager] Start reading the server config file.");

    bool bResult = false;

    CONFIG_MANAGER* pManager = (CONFIG_MANAGER*)pThis;
    pManager->vtbl = s_vtbl;

    // 확장자 검사
    const size_t filenameLength = strlen(pConfigFilePath);
    if (filenameLength < 4 || strcasecmp(&pConfigFilePath[filenameLength - 4], ".ini") != 0)
    {
        ErrorCode_SetLastError(ERROR_CODE_CONFIG_MANAGER_INVALID_CONFIG_FILE_EXTENSION);
        goto lb_return;
    }

    INI_PARSER* pParser = (INI_PARSER*)malloc(sizeof(INI_PARSER));
    if (!INIParser_Init(pParser))
    {
        ErrorCode_SetLastError(ERROR_CODE_CONFIG_MANAGER_FAILED_INIT_PARSER);
        goto lb_return;
    }

    if (!INIParser_Parse(pParser, pConfigFilePath))
    {
        ErrorCode_SetLastError(ERROR_CODE_CONFIG_MANAGER_FAILED_PARSE_INI_FILE);
        goto lb_return;
    }

    if (!INIParser_GetValueShort(pParser, "Server", "httpPort", (short*)&pManager->HttpPort))
    {
        ErrorCode_SetLastError(ERROR_CODE_CONFIG_MANAGER_FAILED_PARSE_HTTP_PORT);
        goto lb_return;
    }

    if (!INIParser_GetValueShort(pParser, "Server", "httpsPort", (short*)&pManager->HttpsPort))
    {
        ErrorCode_SetLastError(ERROR_CODE_CONFIG_MANAGER_FAILED_PARSE_HTTPS_PORT);
        goto lb_return;
    }

    bResult = true;

    Logger_Print(LOG_LEVEL_INFO, "[ConfigManager] End reading the server config file successfully.");
    INIParser_Print(pParser);

lb_return:
    SAFE_FREE(pParser);

    if (!bResult)
    {
        Logger_Print(LOG_LEVEL_ERROR, "[ConfigManager] End reading the server config file with an error.\n"
                                      "Detail: %s", ErrorCode_GetLastErrorDetail());
    }

    return bResult;
}

static void Release(IConfigManager* pThis)
{
    ASSERT(pThis != NULL, "pThis is NULL");

    CONFIG_MANAGER* pManager = (CONFIG_MANAGER*)pThis;
    memset(pManager, 0, sizeof(CONFIG_MANAGER));
}

static uint16_t GetHttpPort(const IConfigManager* pThis)
{
    ASSERT(pThis != NULL, "pThis is NULL");

    CONFIG_MANAGER* pManager = (CONFIG_MANAGER*)pThis;
    return pManager->HttpPort;
}

static uint16_t GetHttpsPort(const IConfigManager* pThis)
{
    ASSERT(pThis != NULL, "pThis is NULL");

    CONFIG_MANAGER* pManager = (CONFIG_MANAGER*)pThis;
    return pManager->HttpsPort;
}

IConfigManager* GetConfigManager(void)
{
    static CONFIG_MANAGER* pManager = NULL;
    if (pManager == NULL)
    {
        pManager = (CONFIG_MANAGER*)malloc(sizeof(CONFIG_MANAGER));
        pManager->vtbl = s_vtbl;
    }

    return (IConfigManager*)pManager;
}