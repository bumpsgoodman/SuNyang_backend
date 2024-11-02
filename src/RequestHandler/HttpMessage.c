// 작성자: bumpsgoodman

#include "HttpMessage.h"
#include "../Common/Assert.h"

#include <stdlib.h>
#include <string.h>

bool ParseStartLine(char* pHttpMessage, START_LINE* pOutStartLine)
{
    ASSERT(pHttpMessage != NULL, "pHttpMessage is NULL");
    ASSERT(pOutStartLine != NULL, "pOutStartLine is NULL");

    static const char* DELIM = " ";

    char* pMethod = strtok(pHttpMessage, DELIM);
    if (pMethod == NULL)
    {
        return false;
    }

    if (strcmp(pMethod, "GET") == 0)
    {
        pOutStartLine->Method = HTTP_METHOD_GET;
    }
    else if (strcmp(pMethod, "HEAD") == 0)
    {
        pOutStartLine->Method = HTTP_METHOD_HEAD;
    }
    else if (strcmp(pMethod, "POST") == 0)
    {
        pOutStartLine->Method = HTTP_METHOD_POST;
    }
    else if (strcmp(pMethod, "PUT") == 0)
    {
        pOutStartLine->Method = HTTP_METHOD_PUT;
    }
    else if (strcmp(pMethod, "DELETE") == 0)
    {
        pOutStartLine->Method = HTTP_METHOD_DELETE;
    }
    else if (strcmp(pMethod, "CONNECT") == 0)
    {
        return false;
    }
    else if (strcmp(pMethod, "OPTIONS") == 0)
    {
        pOutStartLine->Method = HTTP_METHOD_OPTIONS;
    }
    else if (strcmp(pMethod, "TRACE") == 0)
    {
        return false;
    }
    else if (strcmp(pMethod, "PATCH") == 0)
    {
        return false;
    }

    char* pRequestTarget = strtok(NULL, DELIM);
    if (pRequestTarget == NULL)
    {
        return false;
    }

    pOutStartLine->pRequestTarget = pRequestTarget;

    char* pHttpVersion = strtok(NULL, DELIM);
    if (pHttpVersion == NULL)
    {
        return false;
    }

    pOutStartLine->Version.Major = atoi(pHttpVersion + 5);
    pOutStartLine->Version.Minor = atoi(pHttpVersion + 7);

    return true;
}