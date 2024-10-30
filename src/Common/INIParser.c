// 작성자: bumpsgoodman

#include "INIParser.h"
#include "Assert.h"
#include "SafeDelete.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void AddSection(INI_PARSER* pParser, INI_SECTION* pSection);

bool InitINIParser(INI_PARSER* pParser)
{
    ASSERT(pParser != NULL, "pParser is NULL");

    memset(pParser, 0, sizeof(INI_PARSER));
    return true;
}

bool ParseINI(INI_PARSER* pParser, const char* pFilename)
{
    ASSERT(pParser != NULL, "pParser is NULL");
    ASSERT(pFilename != NULL, "pFilename is NULL");

    FILE* pINIFile = fopen(pFilename, "rt");
    if (pINIFile == NULL)
    {
        return false;
    }

    bool bComment = false;
    bool bInSection = false;
    char buffer[1024];
    INI_SECTION* pActiveSection = NULL;
    INI_KEY_VALUE* pActiveKeyValue = NULL;
    while (fgets(buffer, 1024, pINIFile) != NULL)
    {
        char* pStartSection = NULL;
        char* pEndSection = NULL;
        char* pStartKey = NULL;
        char* pEndKey = NULL;
        char* pStartValue = NULL;
        char* pEndValue = NULL;

        const char* p = buffer;
        while (*p != '\0')
        {
            const char c = *p;

            if (c == ';' || c == '#')
            {
                break;
            }
            else if (c == ' ' || c == '\t' || c == '\n')
            {
                p++;
                continue;
            }
            else if (c == '[')
            {
                pStartSection = p + 1;
            }
            else if (c == ']')
            {
                pEndSection = p - 1;
                const size_t sectionLength = pEndSection - pStartSection;

                // 섹션 이름 복사하기
                char* pSectionName = (char*)malloc(sectionLength + 1);
                strncpy(pSectionName, pStartSection, sectionLength);
                pSectionName[sectionLength] = '\0';

                INI_SECTION* pSection = (INI_SECTION*)malloc(sizeof(INI_SECTION));
                pSection->pName = pSectionName;

                AddSection(pParser, pSection);

                pActiveSection = pSection;
                pActiveKeyValue = (INI_KEY_VALUE*)malloc(sizeof(INI_KEY_VALUE));
                break;
            }
            else if (c == '=' || c == ':')
            {
                pEndKey = p - 1;
                const size_t keyLength = pEndKey - pStartKey;

                // 키 복사하기
                char* pKey = (char*)malloc(keyLength + 1);
                strncpy(pKey, pStartKey, keyLength);
                pKey[keyLength] = '\0';

                INI_KEY_VALUE* pKeyValue = (INI_KEY_VALUE*)malloc(sizeof(INI_KEY_VALUE));
                pKeyValue->pKey = pKey;
            }
            else
            {
                if (pActiveSection != NULL)
                {
                    pStartKey = p;
                }
            }

            p++;
        }
    }

    return true;
}

static void AddSection(INI_PARSER* pParser, INI_SECTION* pSection)
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
        pParser->pSectionsListTail = pSection;
    }
}