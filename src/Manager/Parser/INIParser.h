// 작성자: bumpsgoodman

#ifndef __INI_PARSER_H
#define __INI_PARSER_H

#include "Common/PrimitiveType.h"

typedef struct INI_KEY_VALUE
{
    char* pKey;
    char* pValue;

    size_t KeyLength;
    size_t ValueLength;

    struct INI_KEY_VALUE* pPrev;
    struct INI_KEY_VALUE* pNext;
} INI_KEY_VALUE;

typedef struct INI_SECTION
{
    char* pName;
    size_t NameLength;
    
    INI_KEY_VALUE* pKeyValuesListHead;
    INI_KEY_VALUE* pKeyValuesListTail;

    struct INI_SECTION* pPrev;
    struct INI_SECTION* pNext;
} INI_SECTION;

typedef struct INI_PARSER
{
    INI_SECTION* pSectionsListHead;
    INI_SECTION* pSectionsListTail;
} INI_PARSER;

bool INIParser_Init(INI_PARSER* pParser);
void INIParser_Release(INI_PARSER* pParser);
bool INIParser_Parse(INI_PARSER* pParser, const char* pFilename);
void INIParser_Print(const INI_PARSER* pParser);

bool INIParser_GetValueChar(const INI_PARSER* pParser, const char* pSectionName, const char* pKey, char* pOutValue);
bool INIParser_GetValueString(const INI_PARSER* pParser, const char* pSectionName, const char* pKey, char** ppOutValueMalloc);
bool INIParser_GetValueShort(const INI_PARSER* pParser, const char* pSectionName, const char* pKey, short* pOutValue);
bool INIParser_GetValueInt(const INI_PARSER* pParser, const char* pSectionName, const char* pKey, int* pOutValue);
bool INIParser_GetValueFloat(const INI_PARSER* pParser, const char* pSectionName, const char* pKey, float* pOutValue);
bool INIParser_GetValueDouble(const INI_PARSER* pParser, const char* pSectionName, const char* pKey, double* pOutValue);

#endif // __INI_PARSER_H
