// 작성자: bumpsgoodman

#ifndef __STATIC_MEM_POOL_H
#define __STATIC_MEM_POOL_H

#include "../PrimitiveType.h"

#define HEADER_SIZE sizeof(size_t)

typedef struct STATIC_MEM_POOL
{
    size_t          NumElementsPerBlock;
    size_t          NumMaxBlocks;
    size_t          ElementSize;
    size_t          ElementSizeWithHeader;
    size_t          NumBlocks;
    size_t          NumElements;
    size_t          NumMaxElements;

    char**          ppBlocks;
    char***         pppIndexTable;
    char***         pppIndexTablePtr;
} STATIC_MEM_POOL;

bool StaticMemPool_Init(STATIC_MEM_POOL* pPool, const size_t numElementsPerBlock, const size_t numMaxBlocks, const size_t elementSize);
void StaticMemPool_Release(STATIC_MEM_POOL* pPool);

void* StaticMemPool_Alloc(STATIC_MEM_POOL* pPool);
void StaticMemPool_Free(STATIC_MEM_POOL* pPool, void* pMem);
void StaticMemPool_Clear(STATIC_MEM_POOL* pPool);

size_t StaticMemPool_GetNumElementsPerBlock(const STATIC_MEM_POOL* pPool);
size_t StaticMemPool_GetNumMaxBlocks(const STATIC_MEM_POOL* pPool);
size_t StaticMemPool_GetElementSize(const STATIC_MEM_POOL* pPool);
size_t StaticMemPool_GetNumElements(const STATIC_MEM_POOL* pPool);

#endif // __STATIC_MEM_POOL_H