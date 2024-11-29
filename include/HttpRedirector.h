// 작성자: bumpsgoodman

#ifndef __HTTP_REDIRECTOR_H
#define __HTTP_REDIRECTOR_H

#include "Common/PrimitiveType.h"

#include <pthread.h>

// 실패 시, 0 반환
pthread_t HttpRedirector_Start(void);

#endif // __HTTP_REDIRECTOR_H