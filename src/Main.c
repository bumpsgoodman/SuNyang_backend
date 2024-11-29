// 작성자: bumpsgoodman

#include "HttpRedirector.h"
#include "Common/Assert.h"
#include "Common/Defines.h"
#include "Common/PrimitiveType.h"
#include "Common/SafeDelete.h"
#include "Generic/Logger/Logger.h"
#include "Generic/Manager/ConfigManager.h"

#include <dlfcn.h>
#include <stdio.h>

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        Logger_Print(LOG_LEVEL_ERROR, "Unable to read the server config file.");
        goto lb_return;
    }

    bool bResult = false;
    IConfigManager* pConfigManager = GetConfigManager();
    pthread_t httpRedirectorThread = 0;

    // 초기화
    {
        Logger_Print(LOG_LEVEL_INFO, "[main] Start initializing SUNYANGI server.");

        if (!pConfigManager->Init(pConfigManager, argv[1]))
        {
            goto lb_return;
        }

        Logger_Print(LOG_LEVEL_INFO, "[main] End initializing SUNYANGI server successfully.");
    }

    // 실행
    {
        Logger_Print(LOG_LEVEL_INFO, "[main] Start SUNYANGI server.");

        // HttpRedirector 실행
        httpRedirectorThread = HttpRedirector_Start();
        if (httpRedirectorThread == 0)
        {
            goto lb_return;
        }

        // RequestHandler 실행
    }

    pthread_join(httpRedirectorThread, NULL);

    bResult = true;

    Logger_Print(LOG_LEVEL_INFO, "[main] Shutdown SUNYANGI server successfully.");

lb_return:
    // 해제
    {
        SAFE_RELEASE(pConfigManager);
    }

    if (!bResult)
    {
        Logger_Print(LOG_LEVEL_ERROR, "[main] Shutdown SUNYANGI server with an error.");
    }

    return 0;
}
