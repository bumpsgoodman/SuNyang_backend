// 작성자: bumpsgoodman

#ifndef __REQUEST_HANDLER_H
#define __REQUEST_HANDLER_H

#include "Common/PrimitiveType.h"
#include "Common/HttpTypedef.h"

#include <pthread.h>

// 실패 시, 0 반환
pthread_t RequestHandler_Start(void);

// '*'은 매개변수
// 예) "/menu/*/post/*"
bool RequestHandler_RegisterPath(const HTTP_METHOD method, const char* pPath, const size_t pathLength);

#endif // __REQUEST_HANDLER_H