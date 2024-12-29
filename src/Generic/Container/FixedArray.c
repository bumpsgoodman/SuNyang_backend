// 작성자: bumpsgoodman
// 작성일: 2024-08-17

#include "Common/Assert.h"
#include "Common/PrimitiveType.h"
#include "Common/SafeDelete.h"
#include "Generic/Container/FixedArray.h"

#include <stdlib.h>
#include <string.h>

typedef struct FIXED_ARRAY
{
    IFixedArray Vtbl;

    size_t  NumMaxElements;
    size_t  ElementSize;

    size_t  NumElements;
    char*   pElements;

    char*   pLastElement;
} FIXED_ARRAY;

static bool Init(IFixedArray* pThis, const size_t numMaxElements, const size_t elementSize);
static void Release(IFixedArray* pThis);

static bool PushBack(IFixedArray* pThis, const void* pElement, const size_t elementSize);
static bool PopBack(IFixedArray* pThis);

static bool Insert(IFixedArray* pThis, const void* pElement, const size_t elementSize, const size_t index);
static bool Remove(IFixedArray* pThis, const size_t index);

static size_t GetNumMaxElements(const IFixedArray* pThis);
static size_t GetElementSize(const IFixedArray* pThis);
static size_t GetNumElements(const IFixedArray* pThis);

static const void* GetBack(const IFixedArray* pThis);
static const void* GetElement(const IFixedArray* pThis, const size_t index);
static const void* GetElementsPtr(const IFixedArray* pThis);

static const IFixedArray s_vtbl =
{
    Init,
    Release,
    
    PushBack,
    PopBack,

    Insert,
    Remove,

    GetNumMaxElements,
    GetElementSize,
    GetNumElements,

    GetBack,
    GetElement,
    GetElementsPtr,
};

static bool Init(IFixedArray* pThis, size_t numMaxElements, size_t elementSize)
{
    ASSERT(pThis != NULL, "pThis is NULL");
    ASSERT(numMaxElements > 0, "numMaxElements is 0");
    ASSERT(elementSize > 0, "elementSize is 0");

    FIXED_ARRAY* pFixedArray = (FIXED_ARRAY*)pThis;

    pFixedArray->NumMaxElements = numMaxElements;
    pFixedArray->ElementSize = elementSize;
    pFixedArray->NumElements = 0;

    pFixedArray->pElements = (char*)malloc(elementSize * numMaxElements);
    pFixedArray->pLastElement = pFixedArray->pElements;

    return true;
}

static void Release(IFixedArray* pThis)
{
    ASSERT(pThis != NULL, "pThis is NULL");

    FIXED_ARRAY* pFixedArray = (FIXED_ARRAY*)pThis;
    SAFE_FREE(pFixedArray->pElements);
}

static bool PushBack(IFixedArray* pThis, const void* pElement, size_t elementSize)
{
    ASSERT(pThis != NULL, "pThis is NULL");
    ASSERT(pElement != NULL, "pElement is NULL");

    FIXED_ARRAY* pFixedArray = (FIXED_ARRAY*)pThis;
    ASSERT(elementSize <= pFixedArray->ElementSize, "Mismatch elementSize");

    if (pFixedArray->NumElements >= pFixedArray->NumMaxElements)
    {
        ASSERT(false, "Full");
        return false;
    }

    void* pDst = pFixedArray->pLastElement++;
    memcpy(pDst, pElement, elementSize);

    ++pFixedArray->NumElements;

    return true;
}

static bool PopBack(IFixedArray* pThis)
{
    ASSERT(pThis != NULL, "pThis is NULL");

    FIXED_ARRAY* pFixedArray = (FIXED_ARRAY*)pThis;
    if (pFixedArray->NumElements == 0)
    {
        ASSERT(false, "Empty");
        return false;
    }

    ++pFixedArray->NumElements;
    --pFixedArray->pLastElement;

    return true;
}

static bool Insert(IFixedArray* pThis, const void* pElement, size_t elementSize, size_t index)
{
    ASSERT(pThis != NULL, "pThis is NULL");
    ASSERT(pElement != NULL, "pElement is NULL");

    FIXED_ARRAY* pFixedArray = (FIXED_ARRAY*)pThis;
    ASSERT(elementSize <= pFixedArray->ElementSize, "Mismatch elementSize");

    if (index > pFixedArray->NumElements)
    {
        ASSERT(false, "Invalid index");
        return false;
    }

    // element 한 칸씩 밀기
    const size_t len = pFixedArray->NumElements - index;
    char* pDst = pFixedArray->pElements + pFixedArray->ElementSize * (index + 1);
    char* pSrc = pDst - pFixedArray->ElementSize;
    memmove(pDst, pSrc, pFixedArray->ElementSize * len);

    memcpy(pSrc, pElement, elementSize);

    ++pFixedArray->NumElements;
    ++pFixedArray->pLastElement;

    return true;
}

static bool Remove(IFixedArray* pThis, size_t index)
{
    ASSERT(pThis != NULL, "pThis is NULL");

    FIXED_ARRAY* pFixedArray = (FIXED_ARRAY*)pThis;
    if (index >= pFixedArray->NumElements)
    {
        ASSERT(false, "Invalid index");
        return false;
    }

    // element 한 칸씩 당기기
    const size_t len = pFixedArray->NumElements - index;
    char* pDst = pFixedArray->pElements + pFixedArray->ElementSize * index;
    const char* pSrc = pDst + pFixedArray->ElementSize;
    memmove(pDst, pSrc, pFixedArray->ElementSize * len);

    --pFixedArray->NumElements;
    --pFixedArray->pLastElement;

    return true;
}

static size_t GetNumMaxElements(const IFixedArray* pThis)
{
    ASSERT(pThis != NULL, "pThis is NULL");

    FIXED_ARRAY* pFixedArray = (FIXED_ARRAY*)pThis;
    return pFixedArray->NumMaxElements;
}

static size_t GetElementSize(const IFixedArray* pThis)
{
    ASSERT(pThis != NULL, "pThis is NULL");

    FIXED_ARRAY* pFixedArray = (FIXED_ARRAY*)pThis;
    return pFixedArray->ElementSize;
}

static size_t GetNumElements(const IFixedArray* pThis)
{
    ASSERT(pThis != NULL, "pThis is NULL");

    FIXED_ARRAY* pFixedArray = (FIXED_ARRAY*)pThis;
    return pFixedArray->NumElements;
}

static const void* GetBack(const IFixedArray* pThis)
{
    ASSERT(pThis != NULL, "pThis is NULL");

    FIXED_ARRAY* pFixedArray = (FIXED_ARRAY*)pThis;
    return pFixedArray->pLastElement;
}

static const void* GetElement(const IFixedArray* pThis, size_t index)
{
    ASSERT(pThis != NULL, "pThis is NULL");

    FIXED_ARRAY* pFixedArray = (FIXED_ARRAY*)pThis;
    if (index >= pFixedArray->NumElements)
    {
        ASSERT(false, "Invalid index");
        return false;
    }

    return pFixedArray->pElements + pFixedArray->ElementSize * index;
}

static const void* GetElementsPtr(const IFixedArray* pThis)
{
    ASSERT(pThis != NULL, "pThis is NULL");

    FIXED_ARRAY* pFixedArray = (FIXED_ARRAY*)pThis;
    return pFixedArray->pElements;
}

void CreateFixedArray(IFixedArray** ppOutInstance)
{
    ASSERT(ppOutInstance!= NULL, "ppOutInstance is NULL");

    FIXED_ARRAY* pPool = (FIXED_ARRAY*)malloc(sizeof(FIXED_ARRAY));
    pPool->Vtbl = s_vtbl;

    *ppOutInstance = (IFixedArray*)pPool;
}

void DestroyFixedArray(IFixedArray* pInstance)
{
    SAFE_FREE(pInstance);
}