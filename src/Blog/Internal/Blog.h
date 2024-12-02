// 작성자: bumpsgoodman

#ifndef __BLOG_H
#define __BLOG_H

#include "Common/Interface/IBlog.h"
#include "Generic/Container/FixedArray.h"
#include "Generic/MemPool/StaticMemPool.h"

typedef struct BLOG
{
    IBlog vtbl;
    size_t RefCount;

    IStaticMemPool* pPostPool;
    IFixedArray* pPosts;
} BLOG;

static size_t AddRefCount(IBlog* pThis);
static size_t GetRefCount(const IBlog* pThis);
static size_t Release(IBlog* pThis);
static bool Init(IBlog* pThis);

static BLOG_POST_UUID AddPost(IBlog* pThis, const wchar_t* pAuthor, const wchar_t* pTitle, const char* pContent);
static const BLOG_POST* GetPostOrNull(const IBlog* pThis, const BLOG_POST_UUID postId);

#endif // __BLOG_H