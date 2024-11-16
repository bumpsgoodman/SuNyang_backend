#include "ErrorCode.h"
#include "../PrimitiveType.h"

static error_code_t s_errorCode = ERROR_CODE_SUCCESS;

static const char* sp_serverInfoDetailTable[] =
{
    "This file is not an .ini file.",
    "Failed to init ini parser.",
    "Failed to parse ini file.",
    "Failed to get http port.",
    "Failed to get https port.",
};

static const char* sp_HttpRedirectorInfoDetailTable[] =
{
    "Failed to create thread.",
    "Failed to create socket.",
    "Failed to bind socket.",
    "Failed to listen socket.",
    "Failed to create epoll.",
    "Failed to watch epoll descriptor.",
    "Buffer overflow.",
};

static const char** sp_detailTables[] =
{
    sp_serverInfoDetailTable,
    sp_HttpRedirectorInfoDetailTable,
};

void ErrorCode_SetLastError(const error_code_t errorCode)
{
    s_errorCode = errorCode;
}

error_code_t ErrorCode_GetLastError(void)
{
    return s_errorCode;
}

const char* ErrorCode_GetErrorDetail(const error_code_t errorCode)
{
    const uint_t row = (errorCode >> 16) - 1;
    const uint_t col = errorCode & 0xffff;

    const char** pDetailTable = sp_detailTables[row];
    const char* pDetailString = pDetailTable[col];
    return pDetailString;
}

const char* ErrorCode_GetLastErrorDetail(void)
{
    const uint_t row = (s_errorCode >> 16) - 1;
    const uint_t col = s_errorCode & 0xffff;

    const char** pDetailTable = sp_detailTables[row];
    const char* pDetailString = pDetailTable[col];
    return pDetailString;
}