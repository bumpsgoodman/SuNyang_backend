// 작성자: bumpsgoodman

#include "HttpRedirector.h"
#include "Common/Assert.h"
#include "Common/Defines.h"
#include "Common/PrimitiveType.h"
#include "Common/SafeDelete.h"
#include "Common/Interface/IBlog.h"
#include "Generic/Logger/Logger.h"
#include "Generic/Manager/ConfigManager.h"

#include <dlfcn.h>
#include <locale.h>
#include <stdio.h>
#include <wchar.h>

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        Logger_Print(LOG_LEVEL_ERROR, "Unable to read the server config file.");
        goto lb_return;
    }

    setlocale(LC_ALL, "en_US.UTF-8");

    bool bResult = false;
    IConfigManager* pConfigManager = GetConfigManager();
    pthread_t httpRedirectorThread = 0;

    // 초기화
    {
        Logger_Print(LOG_LEVEL_INFO, "[main] Start initializing SUNYANGI server.");

        if (!pConfigManager->Init(pConfigManager, argv[1]))
        {
            goto lb_return;
        }

        Logger_Print(LOG_LEVEL_INFO, "[main] End initializing SUNYANGI server successfully.");
    }

    // 실행
    {
        Logger_Print(LOG_LEVEL_INFO, "[main] Start SUNYANGI server.");

        // HttpRedirector 실행
        httpRedirectorThread = HttpRedirector_Start();
        if (httpRedirectorThread == 0)
        {
            goto lb_return;
        }

        // RequestHandler 실행
    }

    // Blog 테스트
    {
        void* pBlogHandle = NULL;
        CreateInstanceFunc fpCreateBlogInstance = NULL;
        
        pBlogHandle = dlopen("lib/Blog.so", RTLD_NOW | RTLD_GLOBAL);
        if (pBlogHandle == NULL)
        {
            Logger_Print(LOG_LEVEL_ERROR, "[Blog] Failed to open library");
            goto lb_return_blog;
        }

        fpCreateBlogInstance = dlsym(pBlogHandle, "CreateInstance");
        if (fpCreateBlogInstance == NULL)
        {
            Logger_Print(LOG_LEVEL_ERROR, "[Blog] Failed to load CreateInstance funcion");
            goto lb_return_blog;
        }

        IBlog* pBlog;
        fpCreateBlogInstance((void**)&pBlog);

        pBlog->Init(pBlog);

        BLOG_POST_UUID id = pBlog->AddPost(pBlog, L"bumpsgoodman", L"Test First Title", "This is a First posting.");
        const BLOG_POST* pPost = pBlog->GetPostOrNull(pBlog, id);

        Logger_Print(LOG_LEVEL_DEBUG, "[Blog] Test Blog Post");
        fflush(stdout);
        printf("Author: %ls\n", pPost->pAuthor);
        printf("Title: %ls\n", pPost->pTitle);
        fflush(stdout);
        printf("Content: %s\n", pPost->pContent);

        SAFE_RELEASE(pBlog);
        dlclose(pBlogHandle);
    }
lb_return_blog:

    pthread_join(httpRedirectorThread, NULL);

    bResult = true;

    Logger_Print(LOG_LEVEL_INFO, "[main] Shutdown SUNYANGI server successfully.");

lb_return:
    // 해제
    {
        SAFE_RELEASE(pConfigManager);
    }

    if (!bResult)
    {
        Logger_Print(LOG_LEVEL_ERROR, "[main] Shutdown SUNYANGI server with an error.");
    }

    return 0;
}
