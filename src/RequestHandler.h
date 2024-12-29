// 작성자: bumpsgoodman

#ifndef __REQUEST_HANDLER_H
#define __REQUEST_HANDLER_H

#include "Common/PrimitiveType.h"

#include <pthread.h>

// 실패 시, 0 반환
pthread_t RequestHandler_Start(void);

#endif // __REQUEST_HANDLER_H