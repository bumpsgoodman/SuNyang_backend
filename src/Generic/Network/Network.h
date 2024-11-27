#ifndef __NETWORK_H
#define __NETWORK_H

#include "Common/PrimitiveType.h"

#define IPV4_LENGTH 16

typedef enum BYTE_ORDERING {
    BYTE_ORDERING_LITTLE_ENDIAN,
    BYTE_ORDERING_BIG_ENDIAN
} BYTE_ORDERING;

void Network_Ipv4ToString(const uint32_t ipv4, const BYTE_ORDERING ordering, char* pOutIpv4, const BYTE_ORDERING outOrdering);

#endif // __NETWORK_H