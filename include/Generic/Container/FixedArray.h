// 작성자: bumpsgoodman

#ifndef __FIXED_ARRAY
#define __FIXED_ARRAY

#include "Common/Defines.h"
#include "Common/PrimitiveType.h"

typedef INTERFACE IFixedArray IFixedArray;
INTERFACE IFixedArray
{
    bool (*Init)(IFixedArray* pThis, const size_t numMaxElements, const size_t elementSize);
    void (*Release)(IFixedArray* pThis);

    bool (*PushBack)(IFixedArray* pThis, const void* pElement, const size_t elementSize);
    bool (*PopBack)(IFixedArray* pThis);

    bool (*Insert)(IFixedArray* pThis, const void* pElement, const size_t elementSize, const size_t index);
    bool (*Remove)(IFixedArray* pThis, const size_t index);

    size_t (*GetNumMaxElements)(const IFixedArray* pThis);
    size_t (*GetElementSize)(const IFixedArray* pThis);
    size_t (*GetNumElements)(const IFixedArray* pThis);

    const void* (*GetBack)(const IFixedArray* pThis);
    const void* (*GetElement)(const IFixedArray* pThis, const size_t index);
    const void* (*GetElementsPtr)(const IFixedArray* pThis);
};

void CreateFixedArray(IFixedArray** ppOutInstance);
void DestroyFixedArray(IFixedArray* pInstance);

#endif // __FIXED_ARRAY