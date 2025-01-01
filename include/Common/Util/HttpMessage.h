// 작성자: bumpsgoodman

#ifndef __HTTP_MESSAGE_H
#define __HTTP_MESSAGE_H

#include "Common/PrimitiveType.h"
#include "Common/HttpTypedef.h"

const char* GetHttpMethodString(const HTTP_METHOD method);
const char* GetHttpStatusString(const HTTP_STATUS status);

#endif // __HTTP_MESSAGE_H