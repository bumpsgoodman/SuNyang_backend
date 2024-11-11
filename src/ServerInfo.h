// 작성자: bumpsgoodman

#ifndef __SERVER_INFO_H
#define __SERVER_INFO_H

#include "Common/Assert.h"
#include "Common/PrimitiveType.h"
#include "Common/Logger/Logger.h"

typedef struct SERVER_INFO
{
    uint16_t HttpPort;
    uint16_t HttpsPort;
} SERVER_INFO;

bool ServerInfo_Init(SERVER_INFO* pOutServerInfo, const char* pFilename);
void ServerInfo_Release(SERVER_INFO* pServerInfo);

#endif // __SERVER_INFO_H