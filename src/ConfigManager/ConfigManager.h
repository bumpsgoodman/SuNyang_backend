// 작성자: bumpsgoodman

#ifndef __CONFIG_MANAGER_H
#define __CONFIG_MANAGER_H

#include "../Common/Interface/IConfigManager.h"

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

#endif // __CONFIG_MANAGER_H
