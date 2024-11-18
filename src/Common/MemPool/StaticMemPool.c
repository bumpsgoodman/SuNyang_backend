#include "StaticMemPool.h"
#include "../Assert.h"
#include "../SafeDelete.h"

#include <stdlib.h>
#include <string.h>

bool StaticMemPool_Init(STATIC_MEM_POOL* pPool, const size_t numElementsPerBlock, const size_t numMaxBlocks, const size_t elementSize)
{
    ASSERT(pPool != NULL, "pPool is NULL");
    ASSERT(numElementsPerBlock > 0, "numElementsPerBlock is 0");
    ASSERT(numMaxBlocks > 0, "numMaxBlocks is 0");
    ASSERT(elementSize > 0, "elementSize is 0");

    pPool->NumElementsPerBlock = numElementsPerBlock;
    pPool->NumMaxBlocks = numMaxBlocks;
    pPool->ElementSize = elementSize;
    pPool->ElementSizeWithHeader = HEADER_SIZE + elementSize;
    pPool->NumBlocks = 1;
    pPool->NumElements = 0;
    pPool->NumMaxElements = numElementsPerBlock * numMaxBlocks;

    pPool->ppBlocks = (char**)malloc(PTR_SIZE * numMaxBlocks);
    ASSERT(pPool->ppBlocks != NULL, "Failed to malloc");

    memset(pPool->ppBlocks, 0, PTR_SIZE * numMaxBlocks);

    pPool->ppBlocks[0] = (char*)malloc(pPool->ElementSizeWithHeader * numElementsPerBlock);
    ASSERT(pPool->ppBlocks[0] != NULL, "Failed to malloc");

    pPool->pppIndexTable = (char***)malloc(PTR_SIZE * numMaxBlocks);
    ASSERT(pPool->pppIndexTable != NULL, "Failed to malloc");

    char** ppIndexTable = (char**)malloc(PTR_SIZE * numElementsPerBlock);
    ASSERT(ppIndexTable != NULL, "Failed to malloc");

    pPool->pppIndexTablePtr = (char***)malloc(PTR_SIZE * numMaxBlocks);
    ASSERT(pPool->pppIndexTablePtr != NULL, "Failed to malloc");

    // 인덱스 테이블 초기화
    for (size_t i = 0; i < numElementsPerBlock; ++i)
    {
        size_t* pHeader = (size_t*)(pPool->ppBlocks[0] + i * pPool->ElementSizeWithHeader);
        *pHeader = 0;
        ppIndexTable[i] = (char*)pHeader;
    }

    pPool->pppIndexTable[0] = ppIndexTable;
    pPool->pppIndexTablePtr[0] = ppIndexTable;

    return true;
}

void StaticMemPool_Release(STATIC_MEM_POOL* pPool)
{
    ASSERT(pPool != NULL, "pPool is NULL");

    for (size_t i = 0; i < pPool->NumBlocks; ++i)
    {
        SAFE_FREE(pPool->ppBlocks[i]);
        SAFE_FREE(pPool->pppIndexTable[i]);
    }

    SAFE_FREE(pPool->ppBlocks);
    SAFE_FREE(pPool->pppIndexTable);
    SAFE_FREE(pPool->pppIndexTablePtr);

    memset(pPool, 0, sizeof(STATIC_MEM_POOL));
}

void* StaticMemPool_Alloc(STATIC_MEM_POOL* pPool)
{
    ASSERT(pPool != NULL, "pPool is NULL");

    if (pPool->NumElements >= pPool->NumMaxElements)
    {
        ASSERT(false, "Pool is full");
        return NULL;
    }

    // 할당 가능한 블럭 찾기
    size_t blockIndex;
    size_t indexInBlock = 0;
    for (blockIndex = 0; blockIndex < pPool->NumBlocks; ++blockIndex)
    {
        const size_t numAllocElements = (size_t)(pPool->pppIndexTablePtr[blockIndex] - pPool->pppIndexTable[blockIndex]);
        if (numAllocElements < pPool->NumElementsPerBlock)
        {
            ++pPool->pppIndexTablePtr[blockIndex];
            indexInBlock = numAllocElements;
            break;
        }
    }

    // 할당 가능한 블럭이 없으면 블럭 생성
    if (blockIndex >= pPool->NumBlocks)
    {
        char* pBlock = (char*)malloc(pPool->ElementSizeWithHeader * pPool->NumElementsPerBlock);
        ASSERT(pBlock != NULL, "Failed to mallc");

        char** ppIndexTable = (char**)malloc(PTR_SIZE * pPool->NumElementsPerBlock);
        ASSERT(ppIndexTable != NULL, "Failed to malloc");

        // 인덱스 테이블 초기화
        for (size_t i = 0; i < pPool->NumElementsPerBlock; ++i)
        {
            size_t* pHeader = (size_t*)(pBlock + i * pPool->ElementSizeWithHeader);
            *pHeader = blockIndex;
            ppIndexTable[i] = (char*)pHeader;
        }

        pPool->ppBlocks[blockIndex] = pBlock;
        pPool->pppIndexTable[blockIndex] = ppIndexTable;
        pPool->pppIndexTablePtr[blockIndex] = ppIndexTable;
        ++pPool->NumBlocks;
    }

    char* pMem = pPool->pppIndexTable[blockIndex][indexInBlock] + HEADER_SIZE;
    ++pPool->NumElements;

    return pMem;
}

void StaticMemPool_Free(STATIC_MEM_POOL* pPool, void* pMem)
{
    ASSERT(pPool != NULL, "pPool is NULL");

    char* pHeader = (char*)pMem - HEADER_SIZE;
    const size_t blockIndex = *(size_t*)pHeader;
    *(--pPool->pppIndexTablePtr[blockIndex]) = pHeader;

    --pPool->NumElements;
}

void StaticMemPool_Clear(STATIC_MEM_POOL* pPool)
{
    ASSERT(pPool != NULL, "pPool is NULL");

    for (size_t i = 0; i < pPool->NumBlocks; ++i)
    {
        for (size_t j = 0; j < pPool->NumElementsPerBlock; ++j)
        {
            // 헤더 초기화
            size_t* pHeader = (size_t*)(pPool->ppBlocks[i] + j * pPool->ElementSizeWithHeader);
            //*pHeader = i;

            pPool->pppIndexTable[i][j] = (char*)pHeader;
        }

        pPool->pppIndexTablePtr[i] = pPool->pppIndexTable[i];
    }

    pPool->NumElements = 0;
}

size_t StaticMemPool_GetNumElementsPerBlock(const STATIC_MEM_POOL* pPool)
{
    ASSERT(pPool != NULL, "pPool is NULL");
    return pPool->NumElementsPerBlock;
}

size_t StaticMemPool_GetNumMaxBlocks(const STATIC_MEM_POOL* pPool)
{
    ASSERT(pPool != NULL, "pPool is NULL");
    return pPool->NumMaxBlocks;
}

size_t StaticMemPool_GetElementSize(const STATIC_MEM_POOL* pPool)
{
    ASSERT(pPool != NULL, "pPool is NULL");
    return pPool->ElementSize;
}

size_t StaticMemPool_GetNumElements(const STATIC_MEM_POOL* pPool)
{
    ASSERT(pPool != NULL, "pPool is NULL");
    return pPool->NumElements;
}