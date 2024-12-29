// 작성자: bumpsgoodman

#ifndef __INTERPRETER_H
#define __INTERPRETER_H

#include "Common/Interface/IInterpreter.h"

typedef struct INTERPRETER
{
    IInterpreter Vtbl;
    size_t RefCount;
} INTERPRETER;

static size_t AddRefCount(IInterpreter* pThis);
static size_t GetRefCount(const IInterpreter* pThis);
static size_t Release(IInterpreter* pThis);
static bool Init(IInterpreter* pThis);

static bool InterpretRequest(IInterpreter* pThis, const char* pRequestMessage, const size_t requestMessageLength, REQUEST* pOutRequest);

#endif // __INTERPRETER_H