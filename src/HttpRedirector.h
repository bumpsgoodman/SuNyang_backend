// 작성자: bumpsgoodman

#ifndef __HTTP_REDIRECTOR_H
#define __HTTP_REDIRECTOR_H

#include "Common/PrimitiveType.h"

#include <pthread.h>

typedef struct HTTP_REDIRECTOR
{
    pthread_t Thread;
    uint16_t Port;
} HTTP_REDIRECTOR;

bool HttpRedirector_Init(HTTP_REDIRECTOR* pHttpRedirector, uint16_t port);
void HttpRedirector_Start(HTTP_REDIRECTOR* pHttpRedirector);
void HttpRedirector_Shutdown(HTTP_REDIRECTOR* pHttpRedirector);

#endif // __HTTP_REDIRECTOR_H