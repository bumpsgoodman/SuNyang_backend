// 작성자: bumpsgoodman

#include "Common/Assert.h"
#include "Common/SafeDelete.h"
#include "Internal/Blog.h"

#include <stdlib.h>
#include <string.h>
#include <wchar.h>

BLOG_POST_UUID s_post_uuid;

static const IBlog s_vtbl =
{
    AddRefCount,
    GetRefCount,
    Release,
    Init,

    AddPost,
    GetPostOrNull,
};

static size_t AddRefCount(IBlog* pThis)
{
    ASSERT(pThis != NULL, "pThis is NULL");

    BLOG* pBlog = (BLOG*)pThis;
    ++pBlog->RefCount;

    return pBlog->RefCount;
}

static size_t GetRefCount(const IBlog* pThis)
{
    ASSERT(pThis != NULL, "pThis is NULL");

    BLOG* pBlog = (BLOG*)pThis;
    return pBlog->RefCount;
}

static size_t Release(IBlog* pThis)
{
    ASSERT(pThis != NULL, "pThis is NULL");

    BLOG* pBlog = (BLOG*)pThis;
    if (--pBlog->RefCount == 0)
    {
        pBlog->pPostPool->Release(pBlog->pPostPool);

        BLOG_POST* pPosts = (BLOG_POST*)pBlog->pPosts->GetElementsPtr(pBlog->pPosts);
        const size_t numPosts = pBlog->pPosts->GetNumElements(pBlog->pPosts);
        for (size_t i = 0; i < numPosts; ++i)
        {
            SAFE_FREE(pPosts[i].pTitle);
            SAFE_FREE(pPosts[i].pContent);
        }
        pBlog->pPosts->Release(pBlog->pPosts);

        DestroyStaticMemPool(pBlog->pPostPool);
        DestroyFixedArray(pBlog->pPosts);

        SAFE_FREE(pThis);

        return 0;
    }

    return pBlog->RefCount;
}

static bool Init(IBlog* pThis)
{
    ASSERT(pThis != NULL, "pThis is NULL");

    BLOG* pBlog = (BLOG*)pThis;

    CreateStaticMemPool(&pBlog->pPostPool);
    pBlog->pPostPool->Init(pBlog->pPostPool, 10, 10, sizeof(BLOG_POST));

    CreateFixedArray(&pBlog->pPosts);
    pBlog->pPosts->Init(pBlog->pPosts, 100, sizeof(BLOG_POST));

    return true;
}

static BLOG_POST_UUID AddPost(IBlog* pThis, const wchar_t* pAuthor, const wchar_t* pTitle, const char* pContent)
{
    ASSERT(pThis != NULL, "pThis is NULL");
    ASSERT(pAuthor != NULL, "pAuthor is NULL");
    ASSERT(pTitle != NULL, "pTitle is NULL");
    ASSERT(pContent != NULL, "pContent is NULL");

    BLOG* pBlog = (BLOG*)pThis;

    BLOG_POST* pPost = (BLOG_POST*)pBlog->pPostPool->Alloc(pBlog->pPostPool);

    wcscpy(pPost->pAuthor, pAuthor);

    const size_t titleSize = wcslen(pTitle) + 1;
    pPost->pTitle = (wchar_t*)malloc(sizeof(wchar_t) * titleSize);
    wcscpy(pPost->pTitle, pTitle);

    const size_t contentSize = strlen(pContent) + 1;
    pPost->pContent = (char*)malloc(contentSize);
    strcpy(pPost->pContent, pContent);

    pBlog->pPosts->PushBack(pBlog->pPosts, pPost, sizeof(BLOG_POST));

    return s_post_uuid++;
}

static const BLOG_POST* GetPostOrNull(const IBlog* pThis, const BLOG_POST_UUID postId)
{
    ASSERT(pThis != NULL, "pThis is NULL");

    BLOG* pBlog = (BLOG*)pThis;
    if (postId >= pBlog->pPosts->GetNumElements(pBlog->pPosts))
    {
        return NULL;
    }

    const BLOG_POST* pPost = (BLOG_POST*)pBlog->pPosts->GetElement(pBlog->pPosts, postId);
    return pPost;
}

void CreateInstance(void** ppOutInstance)
{
    ASSERT(ppOutInstance != NULL, "ppOutInstance is NULL");

    BLOG* pBlog = (BLOG*)malloc(sizeof(BLOG));
    pBlog->Vtbl = s_vtbl;
    pBlog->RefCount = 1;

    *(IBlog**)ppOutInstance = (IBlog*)pBlog;
}