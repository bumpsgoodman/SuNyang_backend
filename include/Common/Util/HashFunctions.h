#pragma once

#include "Common/PrimitiveType.h"

inline uint32_t Hash32(const char* pBytes, unsigned int length)
{
    uint32_t hash = 0;

    if (length & 0x1)
    {
        hash += *(uint8_t*)pBytes;
        ++pBytes;
        --length;
    }

    if (length & 0x2)
    {
        hash += *(uint16_t*)pBytes;
        pBytes += 2;
        length -= 2;
    }

    for (unsigned int i = 0; i < length; i += 4)
    {
        hash += *(uint32_t*)pBytes;
        pBytes += 4;
    }

    return hash;
}