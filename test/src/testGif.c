#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <DFile.h>
#include "DBitStream.h"
#include "DLog.h"
#include "GlobalConfig.h"

#define TAG     "TestGif"

typedef struct DRGB
{
    unsigned char alpha;
    unsigned char r;
    unsigned char g;
    unsigned char b;
} DRGB;

#define GIF_HEADER_87A  "GIF87a"    // 1987年5月
#define GIF_HEADER_89A  "GIF89a"    // 1989年7月
typedef struct DGif
{
    unsigned char version[32];

    unsigned short logicalWidth;
    unsigned short logicalHeight;

    unsigned int globalColorTableFlag;
    unsigned int colorResolution;
    unsigned int sortFlag;
    unsigned int pixel;
    unsigned char backgroundColor;
    unsigned char pixelAspectRatio;
    DRGB globalColorTable[256];

} DGif;

static DGif* ParseGif(const char *buf, const int size)
{
    if (buf == NULL || size <= 0)
        return NULL;

    DGif *gif = calloc(1, sizeof(DGif));
    if (gif == NULL)
        return NULL;

    void *bs = DBitStreamInit((unsigned char*)buf, size);
    if (bs == NULL)
    {
        free(gif);
        return NULL;
    }

    // parse header
    if (DBitStreamReadBuf(bs, gif->version, strlen(GIF_HEADER_87A)) < 0)
    {
        free(bs);
        free(gif);
        return NULL;
    }
    if (memcmp(gif->version, GIF_HEADER_87A, strlen(GIF_HEADER_87A)) != 0
        && memcmp(gif->version, GIF_HEADER_89A, strlen(GIF_HEADER_89A)) != 0)
        goto ret;

    // parse logical window

    // get width and height
    if (DBitStreamReadShort(bs, &gif->logicalWidth) < 0)
        goto ret;
    if (DBitStreamReadShort(bs, &gif->logicalHeight) < 0)
        goto ret;

    unsigned char ch;
    if (DBitStreamReadChar(bs, &ch) < 0)
        goto ret;
    gif->globalColorTableFlag = (ch & 0x80) >> 7;
    gif->colorResolution = (ch & 0x70) >> 4;
    gif->sortFlag = (ch & 0x08) >> 3;
    gif->pixel = (ch & 0x07);

    if (DBitStreamReadChar(bs, &gif->backgroundColor) < 0)
        goto ret;
    if (DBitStreamReadChar(bs, &gif->pixelAspectRatio) < 0)
        goto ret;

    if (gif->globalColorTableFlag)
    {
        int pixelSize = 1 << (gif->pixel + 1);
        if (DBitStreamGetLeftSize(bs) < pixelSize * 3)
            goto ret;

        for (int i = 0; i < pixelSize; i++)
        {
            unsigned char rgb[3];
            DBitStreamReadBuf(bs, rgb, 3);
            gif->globalColorTable[i].r = rgb[0];
            gif->globalColorTable[i].g = rgb[1];
            gif->globalColorTable[i].b = rgb[2];
        }
    }

ret:
    free(bs);
    return gif;
}

const char *filePath0 = "D:\\study\\CS\\res\\gif\\0.gif";

static void TestParseGif()
{
    // read data from file

    unsigned char *sourceData = NULL;
    size_t sourceLen = 0;

    FILE *fp = fopen(filePath0, "rb+");
    fseek(fp, 0, SEEK_END);
    sourceLen = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    sourceData = malloc(sourceLen);
    fread(sourceData, sourceLen, 1, fp);
    fclose(fp);

    // start to parse Gif

    DGif *gif = ParseGif(sourceData, sourceLen);
    if (gif != NULL)
    {
        free(gif);
    }

    free(sourceData);
}

void TestGif()
{
    // first clear old data
    DLogFlush();

    DLog(DLOG_D, TAG, "Test begin");

    // test eac3 FFmpeg decoder
    TestParseGif();

    DLog(DLOG_D, TAG, "Test end");
}
