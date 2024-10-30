// 작성자: bumpsgoodman

#ifndef __INI_PARSER_H
#define __INI_PARSER_H

#include "PrimitiveType.h"

typedef struct INI_KEY_VALUE
{
    char* pKey;
    void* pValue;

    struct INI_KEY_VALUE* pPrev;
    struct INI_KEY_VALUE* pNext;
} INI_KEY_VALUE;

typedef struct INI_SECTION
{
    char* pName;
    INI_KEY_VALUE* pKeyValuesList;

    struct INI_SECTION* pPrev;
    struct INI_SECTION* pNext;
} INI_SECTION;

typedef struct INI_PARSER
{
    INI_SECTION* pSectionsListHead;
    INI_SECTION* pSectionsListTail;
} INI_PARSER;

bool InitINIParser(INI_PARSER* pParser);
void ReleaseINIParser(INI_PARSER* pParser);
bool ParseINI(INI_PARSER* pParser, const char* pFilename);



#endif // __INI_PARSER_H