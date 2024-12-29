// 작성자: bumpsgoodman

#include "Common/Assert.h"
#include "Common/SafeDelete.h"
#include "Internal/Interpreter.h"
#include "Common/Util/HashFunctions.h"
#include "Generic/ErrorCode/ErrorCode.h"

#include <string.h>

static const IInterpreter s_vtbl =
{
    AddRefCount,
    GetRefCount,
    Release,
    Init,

    InterpretRequest,
};

static HTTP_METHOD getMethod(const char* pMethod);
static bool isValidPath(const char* pPath);

static size_t AddRefCount(IInterpreter* pThis)
{
    ASSERT(pThis != NULL, "pThis is NULL");

    INTERPRETER* pInterpreter = (INTERPRETER*)pThis;
    ++pInterpreter->RefCount;

    return pInterpreter->RefCount;
}

static size_t GetRefCount(const IInterpreter* pThis)
{
    ASSERT(pThis != NULL, "pThis is NULL");

    INTERPRETER* pInterpreter = (INTERPRETER*)pThis;
    return pInterpreter->RefCount;
}

static size_t Release(IInterpreter* pThis)
{
    ASSERT(pThis != NULL, "pThis is NULL");

    INTERPRETER* pInterpreter = (INTERPRETER*)pThis;
    if (--pInterpreter->RefCount == 0)
    {
        SAFE_FREE(pThis);
        return 0;
    }

    return pInterpreter->RefCount;
}

static bool Init(IInterpreter* pThis)
{
    ASSERT(pThis != NULL, "pThis is NULL");

    INTERPRETER* pInterpreter = (INTERPRETER*)pThis;

    return true;
}

static bool InterpretRequest(IInterpreter* pThis, const char* pRequestMessage, const size_t requestMessageLength, REQUEST* pOutRequest)
{
    ASSERT(pThis != NULL, "pThis is NULL");
    ASSERT(pRequestMessage != NULL, "pRequestMessage is NULL");
    ASSERT(requestMessageLength > 0, "Invalid requestMessageLength");
    ASSERT(pOutRequest != NULL, "pOutRequest is NULL");

    static const char* DELIM = " ";

    INTERPRETER* pInterpreter = (INTERPRETER*)pThis;

    bool bResult = false;

    char* pMethod = strtok(pRequestMessage, DELIM);
    if (pMethod == NULL)
    {
        return false;
    }

    // method 파싱
    pOutRequest->Method = getMethod(pMethod);
    if (pOutRequest == HTTP_METHOD_NOT_SUPPORTED)
    {
        goto lb_return;
    }

    // path 파싱
    char* pRequestTarget = strtok(NULL, DELIM);
    if (pRequestTarget == NULL)
    {
        return false;
    }

    pOutStartLine->pRequestTarget = pRequestTarget;

    bResult = true;

lb_return:
    return bResult;
}

void CreateInstance(void** ppOutInstance)
{
    ASSERT(ppOutInstance != NULL, "ppOutInstance is NULL");

    INTERPRETER* pInterpreter = (INTERPRETER*)malloc(sizeof(INTERPRETER));
    pInterpreter->Vtbl = s_vtbl;
    pInterpreter->RefCount = 1;

    *(IInterpreter**)ppOutInstance = (IInterpreter*)pInterpreter;
}

static HTTP_METHOD getMethod(const char* pMethod)
{
    static const uint32_t GET_METHOD_HASH32 = 0x548c;
    static const uint32_t POST_METHOD_HASH32 = 0x54534f50;
    static const uint32_t HEAD_METHOD_HASH32 = 0x44414548;
    static const uint32_t PUT_METHOD_HASH32 = 0x54a5;
    static const uint32_t DELETE_METHOD_HASH32 = 0x45548a90;
    static const uint32_t CONNECT_METHOD_HASH32 = 0x544393e0;
    static const uint32_t OPTIONS_METHOD_HASH32 = 0x534ea3e8;
    static const uint32_t TRACE_METHOD_HASH32 = 0x454341a6;
    static const uint32_t PATCH_METHOD_HASH32 = 0x48435491;

    const size_t methodLength = strlen(pMethod);
    const uint32_t methodHash32 = Hash32(pMethod, methodLength);
    switch (methodHash32)
    {
    case GET_METHOD_HASH32:
        return HTTP_METHOD_GET;
    case POST_METHOD_HASH32:
        return HTTP_METHOD_POST;
    case HEAD_METHOD_HASH32:
        return HTTP_METHOD_HEAD;
    case PUT_METHOD_HASH32:
        return HTTP_METHOD_PUT;
    case DELETE_METHOD_HASH32:
        return HTTP_METHOD_DELETE;
    case OPTIONS_METHOD_HASH32:
        return HTTP_METHOD_OPTIONS;
    case CONNECT_METHOD_HASH32: // fall-through
    case TRACE_METHOD_HASH32:   // fall-through
    case PATCH_METHOD_HASH32:   // fall-through
    default:
        ErrorCode_SetLastError(ERROR_CODE_INTERPRETER_NOT_SUPPORTED_METHOD);
        return HTTP_METHOD_NOT_SUPPORTED;
    }
}