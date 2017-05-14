#include <stdlib.h>
#include <stdio.h>
#include "DFile.h"
#include "DLog.h"

#define TAG     "DFile"

DEXPORT void DFileWrite2Dest(const char *path, const char *buff, int size)
{
    if (path == NULL || buff == NULL || size <= 0)
    {
        return;
    }

    FILE *fp = fopen(path, "ab+");
    if (fp == NULL)
    {
        DLog(DLOG_W, TAG, "open file failed, path is %s", path);
        return;
    }

    fwrite(buff, size, 1, fp);
    fclose(fp);
}

DEXPORT void DFileFlush(const char *path)
{
    FILE *fp = fopen(path, "wb+");
    if (fp == NULL)
    {
        DLog(DLOG_W, TAG, "flush file failed, path is %s", path);
        return;
    }
    fclose(fp);
}
