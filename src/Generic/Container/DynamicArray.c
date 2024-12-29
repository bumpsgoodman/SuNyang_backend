// 작성자: bumpsgoodman
// 작성일: 2024-08-17

#include "Common/Assert.h"
#include "Common/PrimitiveType.h"
#include "Common/SafeDelete.h"
#include "Generic/Container/DynamicArray.h"

#include <stdlib.h>
#include <string.h>

typedef struct DYNAMIC_ARRAY
{
    IDynamicArray Vtbl;

    size_t  NumMaxElements;
    size_t  ElementSize;

    size_t  NumElements;
    char*   pElements;

    char*   pLastElement;
} DYNAMIC_ARRAY;

static bool Init(IDynamicArray* pThis, const size_t numMaxElements, const size_t elementSize);
static void Release(IDynamicArray* pThis);

static bool PushBack(IDynamicArray* pThis, const void* pElement, const size_t elementSize);
static bool PopBack(IDynamicArray* pThis);

static bool Insert(IDynamicArray* pThis, const void* pElement, const size_t elementSize, const size_t index);
static bool Remove(IDynamicArray* pThis, const size_t index);

static size_t GetNumMaxElements(const IDynamicArray* pThis);
static size_t GetElementSize(const IDynamicArray* pThis);
static size_t GetNumElements(const IDynamicArray* pThis);

static const void* GetBack(const IDynamicArray* pThis);
static const void* GetElement(const IDynamicArray* pThis, const size_t index);
static const void* GetElementsPtr(const IDynamicArray* pThis);

static void expand(DYNAMIC_ARRAY* pDynamicArray);

static const IDynamicArray s_vtbl =
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

static bool Init(IDynamicArray* pThis, size_t numMaxElements, size_t elementSize)
{
    ASSERT(pThis != NULL, "pThis is NULL");
    ASSERT(numMaxElements > 0, "numMaxElements is 0");
    ASSERT(elementSize > 0, "elementSize is 0");

    DYNAMIC_ARRAY* pDynamicArray = (DYNAMIC_ARRAY*)pThis;

    pDynamicArray->NumMaxElements = numMaxElements;
    pDynamicArray->ElementSize = elementSize;
    pDynamicArray->NumElements = 0;

    pDynamicArray->pElements = (char*)malloc(elementSize * numMaxElements);
    pDynamicArray->pLastElement = pDynamicArray->pElements;

    return true;
}

static void Release(IDynamicArray* pThis)
{
    ASSERT(pThis != NULL, "pThis is NULL");

    DYNAMIC_ARRAY* pDynamicArray = (DYNAMIC_ARRAY*)pThis;
    SAFE_FREE(pDynamicArray->pElements);
}

static bool PushBack(IDynamicArray* pThis, const void* pElement, size_t elementSize)
{
    ASSERT(pThis != NULL, "pThis is NULL");
    ASSERT(pElement != NULL, "pElement is NULL");

    DYNAMIC_ARRAY* pDynamicArray = (DYNAMIC_ARRAY*)pThis;
    ASSERT(elementSize <= pDynamicArray->ElementSize, "Mismatch elementSize");

    if (pDynamicArray->NumElements >= pDynamicArray->NumMaxElements)
    {
        expand(pDynamicArray);
    }

    void* pDst = pDynamicArray->pLastElement++;
    memcpy(pDst, pElement, elementSize);

    ++pDynamicArray->NumElements;

    return true;
}

static bool PopBack(IDynamicArray* pThis)
{
    ASSERT(pThis != NULL, "pThis is NULL");

    DYNAMIC_ARRAY* pDynamicArray = (DYNAMIC_ARRAY*)pThis;
    if (pDynamicArray->NumElements == 0)
    {
        expand(pDynamicArray);
    }

    ++pDynamicArray->NumElements;
    --pDynamicArray->pLastElement;

    return true;
}

static bool Insert(IDynamicArray* pThis, const void* pElement, size_t elementSize, size_t index)
{
    ASSERT(pThis != NULL, "pThis is NULL");
    ASSERT(pElement != NULL, "pElement is NULL");

    DYNAMIC_ARRAY* pDynamicArray = (DYNAMIC_ARRAY*)pThis;
    ASSERT(elementSize <= pDynamicArray->ElementSize, "Mismatch elementSize");

    if (index > pDynamicArray->NumElements)
    {
        expand(pDynamicArray);
    }

    // element 한 칸씩 밀기
    const size_t len = pDynamicArray->NumElements - index;
    char* pDst = pDynamicArray->pElements + pDynamicArray->ElementSize * (index + 1);
    char* pSrc = pDst - pDynamicArray->ElementSize;
    memmove(pDst, pSrc, pDynamicArray->ElementSize * len);

    memcpy(pSrc, pElement, elementSize);

    ++pDynamicArray->NumElements;
    ++pDynamicArray->pLastElement;

    return true;
}

static bool Remove(IDynamicArray* pThis, size_t index)
{
    ASSERT(pThis != NULL, "pThis is NULL");

    DYNAMIC_ARRAY* pDynamicArray = (DYNAMIC_ARRAY*)pThis;
    if (index >= pDynamicArray->NumElements)
    {
        ASSERT(false, "Invalid index");
        return false;
    }

    // element 한 칸씩 당기기
    const size_t len = pDynamicArray->NumElements - index;
    char* pDst = pDynamicArray->pElements + pDynamicArray->ElementSize * index;
    const char* pSrc = pDst + pDynamicArray->ElementSize;
    memmove(pDst, pSrc, pDynamicArray->ElementSize * len);

    --pDynamicArray->NumElements;
    --pDynamicArray->pLastElement;

    return true;
}

static size_t GetNumMaxElements(const IDynamicArray* pThis)
{
    ASSERT(pThis != NULL, "pThis is NULL");

    DYNAMIC_ARRAY* pDynamicArray = (DYNAMIC_ARRAY*)pThis;
    return pDynamicArray->NumMaxElements;
}

static size_t GetElementSize(const IDynamicArray* pThis)
{
    ASSERT(pThis != NULL, "pThis is NULL");

    DYNAMIC_ARRAY* pDynamicArray = (DYNAMIC_ARRAY*)pThis;
    return pDynamicArray->ElementSize;
}

static size_t GetNumElements(const IDynamicArray* pThis)
{
    ASSERT(pThis != NULL, "pThis is NULL");

    DYNAMIC_ARRAY* pDynamicArray = (DYNAMIC_ARRAY*)pThis;
    return pDynamicArray->NumElements;
}

static const void* GetBack(const IDynamicArray* pThis)
{
    ASSERT(pThis != NULL, "pThis is NULL");

    DYNAMIC_ARRAY* pDynamicArray = (DYNAMIC_ARRAY*)pThis;
    return pDynamicArray->pLastElement;
}

static const void* GetElement(const IDynamicArray* pThis, size_t index)
{
    ASSERT(pThis != NULL, "pThis is NULL");

    DYNAMIC_ARRAY* pDynamicArray = (DYNAMIC_ARRAY*)pThis;
    if (index >= pDynamicArray->NumElements)
    {
        ASSERT(false, "Invalid index");
        return false;
    }

    return pDynamicArray->pElements + pDynamicArray->ElementSize * index;
}

static const void* GetElementsPtr(const IDynamicArray* pThis)
{
    ASSERT(pThis != NULL, "pThis is NULL");

    DYNAMIC_ARRAY* pDynamicArray = (DYNAMIC_ARRAY*)pThis;
    return pDynamicArray->pElements;
}

static void expand(DYNAMIC_ARRAY* pDynamicArray)
{
    ASSERT(pDynamicArray != NULL, "pDynamicArray == NULL");

    char* pNewSpace = (char*)malloc(pDynamicArray->ElementSize * pDynamicArray->NumMaxElements * 2);
    memcpy(pNewSpace, pDynamicArray->pElements, pDynamicArray->ElementSize * pDynamicArray->NumMaxElements);

    free(pDynamicArray->pElements);
    pDynamicArray->pElements = pNewSpace;
    pDynamicArray->NumMaxElements *= 2;

    pDynamicArray->pLastElement = pNewSpace + (pDynamicArray->NumMaxElements - 1);
}

void CreateDynamicArray(IDynamicArray** ppOutInstance)
{
    ASSERT(ppOutInstance!= NULL, "ppOutInstance is NULL");

    DYNAMIC_ARRAY* pPool = (DYNAMIC_ARRAY*)malloc(sizeof(DYNAMIC_ARRAY));
    pPool->Vtbl = s_vtbl;

    *ppOutInstance = (IDynamicArray*)pPool;
}

void DestroyDynamicArray(IDynamicArray* pInstance)
{
    SAFE_FREE(pInstance);
}