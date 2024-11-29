#include "Common/Assert.h"
#include "Common/SafeDelete.h"
#include "Generic/MemPool/StaticMemPool.h"

#include <stdlib.h>
#include <string.h>

#define HEADER_SIZE sizeof(size_t)

static bool Init(IStaticMemPool* pThis, const size_t numElementsPerBlock, const size_t numMaxBlocks, const size_t elementSize);
static void Release(IStaticMemPool* pThis);

static void* Alloc(IStaticMemPool* pThis);
static void Free(IStaticMemPool* pThis, void* pMem);
static void Clear(IStaticMemPool* pThis);

static size_t GetNumElementsPerBlock(const IStaticMemPool* pThis);
static size_t GetNumMaxBlocks(const IStaticMemPool* pThis);
static size_t GetElementSize(const IStaticMemPool* pThis);
static size_t GetNumElements(const IStaticMemPool* pThis);

static const IStaticMemPool s_vtbl =
{
    Init,
    Release,

    Alloc,
    Free,
    Clear,

    GetNumElementsPerBlock,
    GetNumMaxBlocks,
    GetElementSize,
    GetNumElements,
};

typedef struct STATIC_MEM_POOL
{
    IStaticMemPool vtbl;

    size_t NumElementsPerBlock;
    size_t NumMaxBlocks;
    size_t ElementSize;
    size_t ElementSizeWithHeader;
    size_t NumBlocks;
    size_t NumElements;
    size_t NumMaxElements;

    char** ppBlocks;
    char*** pppIndexTable;
    char*** pppIndexTablePtr;
} STATIC_MEM_POOL;

static bool Init(IStaticMemPool* pThis, const size_t numElementsPerBlock, const size_t numMaxBlocks, const size_t elementSize)
{
    ASSERT(pThis != NULL, "pThis is NULL");
    ASSERT(numElementsPerBlock > 0, "numElementsPerBlock is 0");
    ASSERT(numMaxBlocks > 0, "numMaxBlocks is 0");
    ASSERT(elementSize > 0, "elementSize is 0");

    STATIC_MEM_POOL* pPool = (STATIC_MEM_POOL*)pThis;
    pPool->vtbl = s_vtbl;

    pPool->NumElementsPerBlock = numElementsPerBlock;
    pPool->NumMaxBlocks = numMaxBlocks;
    pPool->ElementSize = elementSize;
    pPool->ElementSizeWithHeader = HEADER_SIZE + elementSize;
    pPool->NumBlocks = 1;
    pPool->NumElements = 0;
    pPool->NumMaxElements = numElementsPerBlock * numMaxBlocks;

    pPool->ppBlocks = (char**)malloc(PTR_SIZE * numMaxBlocks);
    memset(pPool->ppBlocks, 0, PTR_SIZE * numMaxBlocks);

    pPool->ppBlocks[0] = (char*)malloc(pPool->ElementSizeWithHeader * numElementsPerBlock);

    pPool->pppIndexTable = (char***)malloc(PTR_SIZE * numMaxBlocks);
    char** ppIndexTable = (char**)malloc(PTR_SIZE * numElementsPerBlock);

    pPool->pppIndexTablePtr = (char***)malloc(PTR_SIZE * numMaxBlocks);

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

static void Release(IStaticMemPool* pThis)
{
    ASSERT(pThis!= NULL, "pThis is NULL");

    STATIC_MEM_POOL* pPool = (STATIC_MEM_POOL*)pThis;

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

static void* Alloc(IStaticMemPool* pThis)
{
    ASSERT(pThis!= NULL, "pThis is NULL");

    STATIC_MEM_POOL* pPool = (STATIC_MEM_POOL*)pThis;

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
        char** ppIndexTable = (char**)malloc(PTR_SIZE * pPool->NumElementsPerBlock);

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

static void Free(IStaticMemPool* pThis, void* pMem)
{
    ASSERT(pThis!= NULL, "pThis is NULL");

    STATIC_MEM_POOL* pPool = (STATIC_MEM_POOL*)pThis;

    char* pHeader = (char*)pMem - HEADER_SIZE;
    const size_t blockIndex = *(size_t*)pHeader;
    *(--pPool->pppIndexTablePtr[blockIndex]) = pHeader;

    --pPool->NumElements;
}

static void Clear(IStaticMemPool* pThis)
{
    ASSERT(pThis!= NULL, "pThis is NULL");

    STATIC_MEM_POOL* pPool = (STATIC_MEM_POOL*)pThis;

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

static size_t GetNumElementsPerBlock(const IStaticMemPool* pThis)
{
    ASSERT(pThis!= NULL, "pThis is NULL");

    STATIC_MEM_POOL* pPool = (STATIC_MEM_POOL*)pThis;
    return pPool->NumElementsPerBlock;
}

static size_t GetNumMaxBlocks(const IStaticMemPool* pThis)
{
    ASSERT(pThis!= NULL, "pThis is NULL");

    STATIC_MEM_POOL* pPool = (STATIC_MEM_POOL*)pThis;
    return pPool->NumMaxBlocks;
}

static size_t GetElementSize(const IStaticMemPool* pThis)
{
    ASSERT(pThis!= NULL, "pThis is NULL");

    STATIC_MEM_POOL* pPool = (STATIC_MEM_POOL*)pThis;
    return pPool->ElementSize;
}

static size_t GetNumElements(const IStaticMemPool* pThis)
{
    ASSERT(pThis!= NULL, "pThis is NULL");

    STATIC_MEM_POOL* pPool = (STATIC_MEM_POOL*)pThis;
    return pPool->NumElements;
}

void CreateStaticMemPool(IStaticMemPool** ppOutInterface)
{
    ASSERT(ppOutInterface!= NULL, "ppOutInterface is NULL");

    STATIC_MEM_POOL* pPool = (STATIC_MEM_POOL*)malloc(sizeof(STATIC_MEM_POOL));
    pPool->vtbl = s_vtbl;

    *ppOutInterface = (IStaticMemPool*)pPool;
}

void DestroyStaticMemPool(IStaticMemPool* pInterface)
{
    SAFE_FREE(pInterface);
}