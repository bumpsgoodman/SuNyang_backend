// 작성자: bumpsgoodman

#ifndef __I_BLOG_H
#define __I_BLOG_H

#include "Common/Defines.h"
#include "Common/PrimitiveType.h"

#define MAX_USER_NAME_LENGTH 32

typedef uint64_t BLOG_POST_UUID;

typedef struct BLOG_POST
{
    wchar_t pAuthor[MAX_USER_NAME_LENGTH];
    wchar_t* pTitle;
    char* pContent;
} BLOG_POST;

typedef INTERFACE IBlog IBlog;
INTERFACE IBlog
{
    size_t (*AddRefCount)(IBlog* pThis);
    size_t (*GetRefCount)(const IBlog* pThis);
    size_t (*Release)(IBlog* pThis);
    bool (*Init)(IBlog* pThis);

    BLOG_POST_UUID (*AddPost)(IBlog* pThis, const wchar_t* pAuthor, const wchar_t* pTitle, const char* pContent);
    const BLOG_POST* (*GetPostOrNull)(const IBlog* pThis, const BLOG_POST_UUID postId);
};

#endif // __I_BLOG_H