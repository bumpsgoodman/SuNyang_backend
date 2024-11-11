// 작성자: bumpsgoodman

#include "INIParser.h"
#include "../Assert.h"
#include "../SafeDelete.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void addSection(INI_PARSER* pParser, INI_SECTION* pSection);
static void addKeyValue(INI_SECTION* pSection, INI_KEY_VALUE* pKeyValue);

static const INI_SECTION* findSectionOrNull(const INI_PARSER* pParser, const char* pSectionName);
static const INI_KEY_VALUE* findKeyValueOrNull(const INI_SECTION* pSection, const char* pKey);

bool INIParser_Init(INI_PARSER* pParser)
{
    ASSERT(pParser != NULL, "pParser is NULL");

    memset(pParser, 0, sizeof(INI_PARSER));
    return true;
}

void INIParser_Release(INI_PARSER* pParser)
{
    ASSERT(pParser != NULL, "pParser is NULL");

    INI_SECTION* pSection = pParser->pSectionsListHead;
    while (pSection != NULL)
    {
        INI_KEY_VALUE* pKeyValue = pSection->pKeyValuesListHead;
        while (pKeyValue != NULL)
        {
            INI_KEY_VALUE* pDeletedKeyValue = pKeyValue;
            pKeyValue = pKeyValue->pNext;

            SAFE_FREE(pDeletedKeyValue->pKey);
            SAFE_FREE(pDeletedKeyValue->pValue);
            SAFE_FREE(pDeletedKeyValue);
        }

        INI_SECTION* pDeletedSection = pSection;
        pSection = pSection->pNext;

        SAFE_FREE(pDeletedSection->pName);
        SAFE_FREE(pDeletedSection);
    }

    memset(pParser, 0, sizeof(INI_PARSER));
}

bool INIParser_Parse(INI_PARSER* pParser, const char* pFilename)
{
    ASSERT(pParser != NULL, "pParser is NULL");
    ASSERT(pFilename != NULL, "pFilename is NULL");

    FILE* pINIFile = fopen(pFilename, "rt");
    if (pINIFile == NULL)
    {
        return false;
    }

    char buffer[1024];
    INI_SECTION* pActiveSection = NULL;
    while (fgets(buffer, 1024, pINIFile) != NULL)
    {
        const char* pStartSection = NULL;
        const char* pEndSection = NULL;
        const char* pStartKey = NULL;
        const char* pEndKey = NULL;
        const char* pStartValue = NULL;
        const char* pEndValue = NULL;

        const char* p = buffer;
        while (*p != '\0')
        {
            const char c = *p;

            if (c == '\n' || c == ';' || c == '#')
            {
                if (pStartValue != NULL)
                {
                    pEndValue = p;
                    break;
                }
            }
            else if (c == '[')
            {
                pStartSection = p + 1;
            }
            else if (c == ']')
            {
                pEndSection = p;
                const size_t sectionLength = pEndSection - pStartSection;

                // 섹션 이름 복사하기
                char* pSectionName = (char*)malloc(sectionLength + 1);
                strncpy(pSectionName, pStartSection, sectionLength);
                pSectionName[sectionLength] = '\0';

                INI_SECTION* pSection = (INI_SECTION*)malloc(sizeof(INI_SECTION));
                pSection->pName = pSectionName;
                pSection->NameLength = sectionLength;
                pSection->pKeyValuesListHead = NULL;
                pSection->pKeyValuesListTail = NULL;

                addSection(pParser, pSection);

                pActiveSection = pSection;
                break;
            }
            else if (c == '=' || c == ':')
            {
                pEndKey = p;
                
            }
            else
            {
                // 섹션 내에 있는 키 찾기 위함
                if (pStartKey == NULL && pActiveSection != NULL)
                {
                    pStartKey = p;
                }

                // 키와 대칭되는 값 찾기 위함
                if (pStartValue == NULL && pEndKey != NULL)
                {
                    pStartValue = p;
                }
            }

            p++;
        }

        if (pStartValue != NULL && pEndValue == NULL)
        {
            pEndValue = p;
        }

        if (pEndValue != NULL)
        {
            // 키 후처리, 공백 제거
            {
                while (*pStartKey == ' ' || *pStartKey == '\t')
                {
                    pStartKey++;
                }

                while (*(pEndKey - 1) == ' ' || *(pEndKey - 1) == '\t')
                {
                    pEndKey--;
                }
            }

            // 값 후처리, 공백 및 따옴표 제거
            {
                while (*pStartValue == ' ' || *pStartValue == '\t'
                    || *pStartValue == '\'' || *pStartValue == '\"')
                {
                    pStartValue++;
                }

                while (*(pEndValue - 1) == ' ' || *(pEndValue - 1) == '\t'
                    || *(pEndValue - 1) == '\'' || *(pEndValue - 1) == '\"')
                {
                    pEndValue--;
                }
            }

            const size_t keyLength = pEndKey - pStartKey;
            const size_t valueLength = pEndValue - pStartValue;

            // 키 복사하기
            char* pKey = (char*)malloc(keyLength + 1);
            strncpy(pKey, pStartKey, keyLength);
            pKey[keyLength] = '\0';

            // 값 복사하기
            char* pValue = (char*)malloc(valueLength + 1);
            strncpy(pValue, pStartValue, valueLength);
            pValue[valueLength] = '\0';

            // 키-값 만들기
            INI_KEY_VALUE* pKeyValue = (INI_KEY_VALUE*)malloc(sizeof(INI_KEY_VALUE));
            pKeyValue->pKey = pKey;
            pKeyValue->pValue = pValue;
            pKeyValue->KeyLength = keyLength;
            pKeyValue->ValueLength = valueLength;

            addKeyValue(pActiveSection, pKeyValue);
        }
    }

    fclose(pINIFile);

    return true;
}

bool INIParser_GetValueChar(const INI_PARSER* pParser, const char* pSectionName, const char* pKey, char* pOutValue)
{
    ASSERT(pParser != NULL, "pParser is NULL");
    ASSERT(pSectionName != NULL, "pSectionName is NULL");
    ASSERT(pKey != NULL, "pKey is NULL");
    ASSERT(pOutValue != NULL, "pOutValue is NULL");

    const INI_SECTION* pSection = findSectionOrNull(pParser, pSectionName);
    if (pSection == NULL)
    {
        return false;
    }

    const INI_KEY_VALUE* pKeyValue = findKeyValueOrNull(pSection, pKey);
    if (pKeyValue == NULL)
    {
        return false;
    }

    *pOutValue = *(char*)pKeyValue->pValue;

    return true;
}

bool INIParser_GetValueString(const INI_PARSER* pParser, const char* pSectionName, const char* pKey, char** ppOutValue)
{
    ASSERT(pParser != NULL, "pParser is NULL");
    ASSERT(pSectionName != NULL, "pSectionName is NULL");
    ASSERT(pKey != NULL, "pKey is NULL");
    ASSERT(ppOutValue != NULL, "ppOutValue is NULL");

    const INI_SECTION* pSection = findSectionOrNull(pParser, pSectionName);
    if (pSection == NULL)
    {
        return false;
    }

    const INI_KEY_VALUE* pKeyValue = findKeyValueOrNull(pSection, pKey);
    if (pKeyValue == NULL)
    {
        return false;
    }

    char* pValue = (char*)malloc(pKeyValue->ValueLength + 1);
    strcpy(pValue, pKeyValue->pValue);

    *ppOutValue = pValue;

    return true;
}

bool INIParser_GetValueShort(const INI_PARSER* pParser, const char* pSectionName, const char* pKey, short* pOutValue)
{
    ASSERT(pParser != NULL, "pParser is NULL");
    ASSERT(pSectionName != NULL, "pSectionName is NULL");
    ASSERT(pKey != NULL, "pKey is NULL");
    ASSERT(pOutValue != NULL, "pOutValue is NULL");

    const INI_SECTION* pSection = findSectionOrNull(pParser, pSectionName);
    if (pSection == NULL)
    {
        return false;
    }

    const INI_KEY_VALUE* pKeyValue = findKeyValueOrNull(pSection, pKey);
    if (pKeyValue == NULL)
    {
        return false;
    }

    *pOutValue = (short)atoi(pKeyValue->pValue);

    return true;
}

bool INIParser_GetValueInt(const INI_PARSER* pParser, const char* pSectionName, const char* pKey, int* pOutValue)
{
    ASSERT(pParser != NULL, "pParser is NULL");
    ASSERT(pSectionName != NULL, "pSectionName is NULL");
    ASSERT(pKey != NULL, "pKey is NULL");
    ASSERT(pOutValue != NULL, "pOutValue is NULL");

    const INI_SECTION* pSection = findSectionOrNull(pParser, pSectionName);
    if (pSection == NULL)
    {
        return false;
    }

    const INI_KEY_VALUE* pKeyValue = findKeyValueOrNull(pSection, pKey);
    if (pKeyValue == NULL)
    {
        return false;
    }

    *pOutValue = atoi(pKeyValue->pValue);

    return true;
}

bool INIParser_GetValueFloat(const INI_PARSER* pParser, const char* pSectionName, const char* pKey, float* pOutValue)
{
    ASSERT(pParser != NULL, "pParser is NULL");
    ASSERT(pSectionName != NULL, "pSectionName is NULL");
    ASSERT(pKey != NULL, "pKey is NULL");
    ASSERT(pOutValue != NULL, "pOutValue is NULL");

    const INI_SECTION* pSection = findSectionOrNull(pParser, pSectionName);
    if (pSection == NULL)
    {
        return false;
    }

    const INI_KEY_VALUE* pKeyValue = findKeyValueOrNull(pSection, pKey);
    if (pKeyValue == NULL)
    {
        return false;
    }

    *pOutValue = (float)atof(pKeyValue->pValue);

    return true;
}

bool INIParser_GetValueDouble(const INI_PARSER* pParser, const char* pSectionName, const char* pKey, double* pOutValue)
{
    ASSERT(pParser != NULL, "pParser is NULL");
    ASSERT(pSectionName != NULL, "pSectionName is NULL");
    ASSERT(pKey != NULL, "pKey is NULL");
    ASSERT(pOutValue != NULL, "pOutValue is NULL");

    const INI_SECTION* pSection = findSectionOrNull(pParser, pSectionName);
    if (pSection == NULL)
    {
        return false;
    }

    const INI_KEY_VALUE* pKeyValue = findKeyValueOrNull(pSection, pKey);
    if (pKeyValue == NULL)
    {
        return false;
    }

    *pOutValue = atof(pKeyValue->pValue);

    return true;
}

static void addSection(INI_PARSER* pParser, INI_SECTION* pSection)
{
    ASSERT(pParser != NULL, "pParser is NULL");
    ASSERT(pSection != NULL, "pSection is NULL");
    
    if (pParser->pSectionsListTail == NULL)
    {
        pParser->pSectionsListTail = pParser->pSectionsListHead = pSection;
        pSection->pNext = pSection->pPrev = NULL;
    }
    else
    {
        pParser->pSectionsListTail->pNext = pSection;
        pSection->pPrev = pParser->pSectionsListTail;
        pSection->pNext = NULL;
        pParser->pSectionsListTail = pSection;
    }
}

static void addKeyValue(INI_SECTION* pSection, INI_KEY_VALUE* pKeyValue)
{
    ASSERT(pSection != NULL, "pSection is NULL");
    ASSERT(pKeyValue != NULL, "pKeyValue is NULL");

    if (pSection->pKeyValuesListTail == NULL)
    {
        pSection->pKeyValuesListTail = pSection->pKeyValuesListHead = pKeyValue;
        pKeyValue->pNext = pKeyValue->pPrev = NULL;
    }
    else
    {
        pSection->pKeyValuesListTail->pNext = pKeyValue;
        pKeyValue->pPrev = pSection->pKeyValuesListTail;
        pKeyValue->pNext = NULL;
        pSection->pKeyValuesListTail = pKeyValue;
    }
}

static const INI_SECTION* findSectionOrNull(const INI_PARSER* pParser, const char* pSectionName)
{
    ASSERT(pParser != NULL, "pParser is NULL");
    ASSERT(pSectionName != NULL, "pSectionName is NULL");

    const INI_SECTION* pSection = pParser->pSectionsListHead;
    while (pSection != NULL)
    {
        if (strcmp(pSection->pName, pSectionName) == 0)
        {
            return pSection;
        }

        pSection = pSection->pNext;
    }

    return NULL;
}

static const INI_KEY_VALUE* findKeyValueOrNull(const INI_SECTION* pSection, const char* pKey)
{
    ASSERT(pSection != NULL, "pSection is NULL");
    ASSERT(pKey != NULL, "pKey is NULL");

    const INI_KEY_VALUE* pKeyValue = pSection->pKeyValuesListHead;
    while (pKeyValue != NULL)
    {
        if (strcmp(pKeyValue->pKey, pKey) == 0)
        {
            return pKeyValue;
        }

        pKeyValue = pKeyValue->pNext;
    }

    return NULL;
}