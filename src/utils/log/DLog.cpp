#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include "DLog.h"
#include "DLogCommon.h"

void DLogOutput(DLogMode logMode, DLogLevel level, const char *tag, const char *buf);

static DLogMode gLogMode = DLOG_CONSOLE;

void DLogOutputMode(DLogMode logMode)
{
    gLogMode = logMode;
}

void DLog(DLogLevel level, const char *tag, const char *format, ...)
{
    if (level < DLOG_D || level > DLOG_E || level < LOG_OUTPUT_LEVEL)
    {
        return;
    }

    if (!(gLogMode & 0x11))
    {
        return;
    }

    char buf[LOG_BUF_SIZE];
    va_list ap;
    va_start(ap, format);
    int size = vsnprintf(buf, LOG_BUF_SIZE, format, ap);
    va_end(ap);

    if (size <= 0)
    {
        return;
    }

    DLogOutput(gLogMode, level, tag, buf);
}
