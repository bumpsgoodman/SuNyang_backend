// 작성자: bumpsgoodman

#ifndef __I_INTERPRETER_H
#define __I_INTERPRETER_H

#include "Common/Defines.h"
#include "Common/HttpTypedef.h"
#include "Common/PrimitiveType.h"

typedef INTERFACE IInterpreter IInterpreter;
INTERFACE IInterpreter
{
    size_t (*AddRefCount)(IInterpreter* pThis);
    size_t (*GetRefCount)(const IInterpreter* pThis);
    size_t (*Release)(IInterpreter* pThis);
    bool (*Init)(IInterpreter* pThis);

    bool (*InterpretRequest)(IInterpreter* pThis, const char* pRequestMessage, const size_t requestMessageLength, REQUEST* pOutRequest);
};

#endif // __I_INTERPRETER_H