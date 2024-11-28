// 작성자: bumpsgoodman

#ifndef __STATIC_MEM_POOL_H
#define __STATIC_MEM_POOL_H

#include "Common/Defines.h"
#include "Common/PrimitiveType.h"

typedef INTERFACE IStaticMemPool IStaticMemPool;
INTERFACE IStaticMemPool
{
    bool (*Init)(IStaticMemPool* pThis, const size_t numElementsPerBlock, const size_t numMaxBlocks, const size_t elementSize);
    void (*Release)(IStaticMemPool* pThis);

    void* (*Alloc)(IStaticMemPool* pThis);
    void (*Free)(IStaticMemPool* pThis, void* pMem);
    void (*Clear)(IStaticMemPool* pThis);

    size_t (*GetNumElementsPerBlock)(const IStaticMemPool* pThis);
    size_t (*GetNumMaxBlocks)(const IStaticMemPool* pThis);
    size_t (*GetElementSize)(const IStaticMemPool* pThis);
    size_t (*GetNumElements)(const IStaticMemPool* pThis);
};

void CreateStaticMemPool(IStaticMemPool** ppOutInterface);
void DestroyStaticMemPool(IStaticMemPool* pInterface);

#endif // __STATIC_MEM_POOL_H