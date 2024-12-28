// 작성자: bumpsgoodman

#ifndef __HTTP_INTERPRETER_H
#define __HTTP_INTERPRETER_H

#include "Common/PrimitiveType.h"

#include <pthread.h>

// 실패 시, 0 반환
pthread_t HttpInterpreter_Start(void);

#endif // __HTTP_INTERPRETER_H