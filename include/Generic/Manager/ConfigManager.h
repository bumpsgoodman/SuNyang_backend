// 작성자: bumpsgoodman

#ifndef __CONFIG_MANAGER_H
#define __CONFIG_MANAGER_H

#include "Common/Defines.h"
#include "Common/PrimitiveType.h"

typedef INTERFACE IConfigManager IConfigManager;
INTERFACE IConfigManager
{
    bool (*Init)(IConfigManager* pThis, const char* pConfigFilePath);
    void (*Release)(IConfigManager* pThis);

    uint16_t (*GetHttpPort)(const IConfigManager* pThis);
    uint16_t (*GetHttpsPort)(const IConfigManager* pThis);
    const char* (*GetCertPath)(const IConfigManager* pThis);
    const char* (*GetPrivateKeyPath)(const IConfigManager* pThis);
};

IConfigManager* GetConfigManager(void);

#endif // __CONFIG_MANAGER_H