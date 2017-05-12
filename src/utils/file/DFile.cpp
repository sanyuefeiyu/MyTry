#include <stdlib.h>
#include <stdio.h>
#include "DFile.h"
#include "DLog.h"

void DFileWrite2Dest(const char *filePath, const char *buff, const int size)
{
    if (filePath == NULL || buff == NULL || size <= 0)
    {
        return;
    }

    FILE *fp = fopen(filePath, "ab+");
    if (fp == NULL)
    {
        DLog(DLOG_W, "File", "open file failed, filePath=%s", filePath);
        return;
    }

    fwrite(buff, size, 1, fp);
    fclose(fp);
}

void DFileFlush(const char *filePath)
{
    FILE *fp = fopen(filePath, "wb+");
    if (fp == NULL)
    {
        return;
    }
    fclose(fp);
}
