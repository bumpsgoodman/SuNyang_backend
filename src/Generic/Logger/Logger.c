// 작성자: bumpsgoodman

#include "Logger.h"
#include "Common/Assert.h"
#include "Common/ConsoleVTSequence.h"
#include "Common/PrimitiveType.h"

#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#define ESC "\x1b"

void Logger_Print(const LOG_LEVEL level, const char* pFormat, ...)
{
    ASSERT(pFormat != NULL, "pFormat is NULL");

    time_t now = time(NULL);
    char buf[20];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
    printf("%s - ", buf);

    switch (level)
    {
    case LOG_LEVEL_DEBUG:
        CONSOLE_VT_SET_TEXT_COLOR(0, 255, 255);
        printf("[DEBUG] ");
        break;
    case LOG_LEVEL_INFO:
        CONSOLE_VT_SET_TEXT_COLOR(0, 255, 0);
        printf("[INFO] ");
        break;
    case LOG_LEVEL_WARNING:
        CONSOLE_VT_SET_TEXT_COLOR(255, 255, 0);
        printf("[WARNING] ");
        break;
    case LOG_LEVEL_ERROR:
        CONSOLE_VT_SET_TEXT_COLOR(255, 0, 0);
        printf("[ERROR] ");
        break;
    default:
        ASSERT(false, "Invalid log level");
        break;
    }

    CONSOLE_VT_SET_DEFAULT_COLOR();

    va_list args;
    va_start(args, pFormat);
    vprintf(pFormat, args);
    printf("\n");
    va_end(args);
}

void Logger_PrintFile(const LOG_LEVEL level, const char* pFilename, const char* pFormat, ...)
{
    ASSERT(pFormat != NULL, "pFormat is NULL");
    ASSERT(pFilename != NULL, "pFilename is NULL");

    FILE* pFile = fopen(pFilename, "a");
    if (pFile == NULL)
    {
        return;
    }

    time_t now = time(NULL);
    char buf[20];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
    fprintf(pFile, "%s - ", buf);

    switch (level)
    {
    case LOG_LEVEL_DEBUG:
        fprintf(pFile, "[DEBUG] ");
        break;
    case LOG_LEVEL_INFO:
        fprintf(pFile, "[INFO] ");
        break;
    case LOG_LEVEL_WARNING:
        fprintf(pFile, "[WARNING] ");
        break;
    case LOG_LEVEL_ERROR:
        fprintf(pFile, "[ERROR] ");
        break;
    default:
        ASSERT(false, "Invalid log level");
        break;
    }

    va_list args;
    va_start(args, pFormat);
    vfprintf(pFile, pFormat, args);
    fprintf(pFile, "\n");
    va_end(args);
}