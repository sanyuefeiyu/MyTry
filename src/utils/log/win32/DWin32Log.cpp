#include <stdio.h>
#include <stdarg.h>
#include "windows.h"
#include "DLogCommon.h"
#include "DLog.h"
#include "DFile.h"

#define LOG_FILE_FULL_NAME  "D:\\DLog.txt"

void DLogOutput(DLogMode logMode, DLogLevel level, const char *tag, const char *buf)
{
    char outputBuf[LOG_BUF_SIZE + 256];

    // get date time
    SYSTEMTIME  lt;
    GetLocalTime(&lt);

    // date time pid tid level tag msg
    int size = snprintf(outputBuf, LOG_BUF_SIZE + 256, "%04d-%02d-%02d %02d:%02d:%02d:%03d [%d|%d] [%s:%s] %s %s\r\n",
                        lt.wYear, lt.wMonth, lt.wDay, lt.wHour, lt.wMinute, lt.wSecond, lt.wMilliseconds,
                        GetCurrentProcessId(), GetCurrentThreadId(),
                        TAG_APP, tag, gLogLevelDes[level], buf);

    if (size <= 0)
    {
        return;
    }

    if (logMode & DLOG_CONSOLE)
    {
        // write log to console
        OutputDebugStringA(outputBuf);
    }

    if (logMode & DLOG_FILE)
    {
        FILE *fp = fopen(LOG_FILE_FULL_NAME, "ab+");
        if (fp == NULL)
        {
            return;
        }
        fwrite(outputBuf, size, 1, fp);
        fclose(fp);
    }
}

void DLogFlush()
{
	DFileFlush(LOG_FILE_FULL_NAME);
}
