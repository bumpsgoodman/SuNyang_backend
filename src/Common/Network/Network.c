#include "Network.h"
#include "../Assert.h"

#include <stdlib.h>

void Network_Ipv4ToString(const uint32_t ipv4, const BYTE_ORDERING ordering, char* pOutIpv4, const BYTE_ORDERING outOrdering)
{
    ASSERT(pOutIpv4 != NULL, "pOutIpv4 is NULL");

    uint8_t a;
    uint8_t b;
    uint8_t c;
    uint8_t d;

    switch (ordering)
    {
    case BYTE_ORDERING_LITTLE_ENDIAN:
    {
        a = ipv4 & 0xff;
        b = (ipv4 & 0xff00) >> 8;
        c = (ipv4 & 0xff0000) >> 16;
        d = (ipv4 & 0xff000000) >> 24;
        break;
    }
    case BYTE_ORDERING_BIG_ENDIAN:
    {
        a = (ipv4 & 0xff000000) >> 24;
        b = (ipv4 & 0xff0000) >> 16;
        c = (ipv4 & 0xff00) >> 8;
        d = ipv4 & 0xff;
        break;
    }
    default:
        ASSERT(false, "Invalid byte ordering.");
        break;
    }

    switch (outOrdering)
    {
    case BYTE_ORDERING_LITTLE_ENDIAN:
    {
        sprintf(pOutIpv4, "%d.%d.%d.%d", d, c, b, a);
        break;
    }
    case BYTE_ORDERING_BIG_ENDIAN:
    {
        sprintf(pOutIpv4, "%d.%d.%d.%d", a, b, c, d);
        break;
    }
    default:
        ASSERT(false, "Invalid out byte ordering.");
        break;
    }
}