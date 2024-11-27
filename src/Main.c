// 작성자: bumpsgoodman

#include "Common/Assert.h"
#include "Common/PrimitiveType.h"
#include "Common/SafeDelete.h"
#include "Common/Interface/IConfigManager.h"
#include "Generic/Logger/Logger.h"

#include <dlfcn.h>
#include <stdio.h>

typedef void* (*GetManagerFunc)(void);

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        Logger_Print(LOG_LEVEL_ERROR, "Unable to read the server config file.");
        goto lb_return;
    }

    void* pManagerLibHandle = dlopen("../output/dynamic_lib/Manager.so", RTLD_NOW | RTLD_GLOBAL);
    if (pManagerLibHandle == NULL)
    {
        fprintf(stderr, "Unable to open lib: %s\n", dlerror());
        return -1;
    }
    GetManagerFunc fpGetConfigManager = (void*)dlsym(pManagerLibHandle, "GetConfigManager");
    if (fpGetConfigManager == NULL)
    {
        fprintf(stderr, "Unable to get symbol");
        return -1;
    }

    bool bResult = false;
    IConfigManager* pConfigManager = fpGetConfigManager();

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
    }

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
