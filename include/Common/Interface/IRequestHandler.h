// 작성자: bumpsgoodman

#ifndef __I_REQUEST_HANDLER_H
#define __I_REQUEST_HANDLER_H

#include "Common/Defines.h"
#include "Common/PrimitiveType.h"

typedef INTERFACE IRequestHandler IRequestHandler;
INTERFACE IRequestHandler
{
    size_t (*AddRefCount)(IRequestHandler* pThis);
    size_t (*GetRefCount)(const IRequestHandler* pThis);
    size_t (*Release)(IRequestHandler* pThis);
    bool (*Init)(IRequestHandler* pThis);

    
};

#endif // __I_REQUEST_HANDLER_H