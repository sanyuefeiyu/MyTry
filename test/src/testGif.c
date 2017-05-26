#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <DFile.h>
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

    unsigned int logicalWidth;
    unsigned int logicalHeight;

    unsigned int globalColorTableFlag;
    unsigned int colorResolution;
    unsigned int sortFlag;
    unsigned int pixel;
    unsigned int backgroundColor;
    unsigned int pixelAspectRatio;
    DRGB globalColorTable[256];

} DGif;

static DGif* ParseGif(const char *buf, const int size)
{
    if (buf == NULL || size <= 0)
    {
        return NULL;
    }

    DGif *gif = calloc(1, sizeof(DGif));
    if (gif == NULL)
    {
        return NULL;
    }

    // parse header

    const char *tempBuf = buf;
    int pos = 0;
    if (memcmp(tempBuf, GIF_HEADER_87A, strlen(GIF_HEADER_87A)) == 0)
    {
        memcpy(gif->version, GIF_HEADER_87A, strlen(GIF_HEADER_87A));
        pos += strlen(GIF_HEADER_87A);
    }
    else if (memcmp(tempBuf, GIF_HEADER_89A, strlen(GIF_HEADER_89A)) == 0)
    {
        memcpy(gif->version, GIF_HEADER_89A, strlen(GIF_HEADER_89A));
        pos += strlen(GIF_HEADER_89A);
    }
    else
    {
        free(gif);
        return NULL;
    }

    if (pos + 7 > size)
    {
        return gif;
    }

    // parse logical window

    // get width and height
    unsigned char *pWidth = (unsigned char*)(&gif->logicalWidth);
    pWidth[0] = *(tempBuf + pos + 0);
    pWidth[1] = *(tempBuf + pos + 1);
    pos += 2;
    unsigned char *pHeight = (unsigned char*)(&gif->logicalHeight);
    pHeight[0] = *(tempBuf + pos + 0);
    pHeight[1] = *(tempBuf + pos + 1);
    pos += 2;

    unsigned char ch = *(tempBuf + pos);
    gif->globalColorTableFlag = (ch & 0x80) >> 7;
    gif->colorResolution = (ch & 0x70) >> 4;
    gif->sortFlag = (ch & 0x08) >> 3;
    gif->pixel = (ch & 0x07);
    pos += 1;

    gif->backgroundColor = *(tempBuf + pos);
    pos += 1;
    gif->pixelAspectRatio = *(tempBuf + pos);
    pos += 1;

    if (gif->globalColorTableFlag)
    {
        int pixelSize = 1 << (gif->pixel + 1);
        if (pos + pixelSize * 3 > size)
        {
            return gif;
        }

        for (int i = 0; i < pixelSize; i++)
        {
            gif->globalColorTable[i].r = *(tempBuf + pos + i + 0);
            gif->globalColorTable[i].g = *(tempBuf + pos + i + 1);
            gif->globalColorTable[i].b = *(tempBuf + pos + i + 2);
        }

        pos += (pixelSize * 3);
    }

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
