#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>
#include <android/log.h>
#include "DLogCommon.h"
#include "DLog.h"
#include "DFile.h"

#define LOG_FILE_FULL_NAME  "/sdcard/DLog.txt"

void DLogOutput(DLogMode logMode, DLogLevel level, const char *tag, const char *buf)
{
    struct timeval tv;
    struct timezone tz;
    gettimeofday(&tv, &tz);
    time_t currTime = time(NULL);
    tm tm2 = *localtime(&currTime);

    if (logMode & DLOG_CONSOLE)
    {
        // write log to ddms
        __android_log_print(level + 3, TAG_APP, "%s: %s", tag, buf);
    }

    if (logMode & DLOG_FILE)
    {
        // write log to file
        char outputBuf[LOG_BUF_SIZE+256];
        int size = snprintf(outputBuf, LOG_BUF_SIZE+256, "%04d-%02d-%02d %02d:%02d:%02d:%03ld [%s:%s] %s %s\r\n",
                            tm2.tm_year+1900, tm2.tm_mon+1, tm2.tm_mday, tm2.tm_hour, tm2.tm_min, tm2.tm_sec, tv.tv_usec/1000,
                            TAG_APP, tag, gLogLevelDes[level], buf);
        if (size <= 0)
        {
            return;
        }

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
