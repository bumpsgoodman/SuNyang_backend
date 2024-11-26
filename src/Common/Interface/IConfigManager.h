// 작성자: bumpsgoodman

#ifndef __I_CONFIG_MANAGER_H
#define __I_CONFIG_MANAGER_H

#include "../Defines.h"
#include "../PrimitiveType.h"

typedef struct SERVER_INFO
{
    size_t RefCount;

    uint16_t HttpPort;
    uint16_t HttpsPort;
} SERVER_INFO;

typedef INTERFACE IConfigManager IConfigManager;
INTERFACE IConfigManager
{
    bool (*Init)(IConfigManager* pThis, const char* pConfigFilePath);
    void (*Release)(IConfigManager* pThis);

    uint16_t (*GetHttpPort)(const IConfigManager* pThis);
    uint16_t (*GetHttpsPort)(const IConfigManager* pThis);
};

IConfigManager* GetConfigManager(void);

#endif // __I_CONFIG_MANAGER_H