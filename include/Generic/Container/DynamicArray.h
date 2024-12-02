// 작성자: bumpsgoodman

#ifndef __DYNAMIC_ARRAY_H
#define __DYNAMIC_ARRAY_H

#include "Common/Defines.h"
#include "Common/PrimitiveType.h"

typedef INTERFACE IDynamicArray IDynamicArray;
INTERFACE IDynamicArray
{
    bool (*Init)(IDynamicArray* pThis, const size_t numMaxElements, const size_t elementSize);
    void (*Release)(IDynamicArray* pThis);

    bool (*PushBack)(IDynamicArray* pThis, const void* pElement, const size_t elementSize);
    bool (*PopBack)(IDynamicArray* pThis);

    bool (*Insert)(IDynamicArray* pThis, const void* pElement, const size_t elementSize, const size_t index);
    bool (*Remove)(IDynamicArray* pThis, const size_t index);

    size_t (*GetNumMaxElements)(const IDynamicArray* pThis);
    size_t (*GetElementSize)(const IDynamicArray* pThis);
    size_t (*GetNumElements)(const IDynamicArray* pThis);

    const void* (*GetBack)(const IDynamicArray* pThis);
    const void* (*GetElement)(const IDynamicArray* pThis, const size_t index);
    const void* (*GetElementsPtr)(const IDynamicArray* pThis);
};

void CreateDynamicArray(IDynamicArray** ppOutInstance);
void DestroyDynamicArray(IDynamicArray* pInstance);

#endif // __DYNAMIC_ARRAY_H