// 작성자: bumpsgoodman

#ifndef __LOGGER_H
#define __LOGGER_H

typedef enum LOG_LEVEL
{
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR
} LOG_LEVEL;

void Logger_Print(const LOG_LEVEL level, const char* pFormat, ...);
void Logger_PrintFile(const LOG_LEVEL level, const char* pFilename, const char* pFormat, ...);

#endif // __LOGGER_H