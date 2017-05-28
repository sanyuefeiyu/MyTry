#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <DFile.h>
#include "DBitStream.h"
#include "DLog.h"
#include "GlobalConfig.h"

#define TAG     "TestGif"

#define Magic_Image_Descriptor        0x2C
#define Magic_Extension_Introducer    0x21

typedef struct DRGB
{
    unsigned char alpha;
    unsigned char r;
    unsigned char g;
    unsigned char b;
} DRGB;

typedef struct LocalColorTableFlag
{
    unsigned int m;
    unsigned int i;
    unsigned int s;
    unsigned int r;
    unsigned int pixel;
} LocalColorTableFlag;

typedef struct ImageDescriptor
{
    unsigned char magic;
    unsigned short offsetX;
    unsigned short offsetY;
    unsigned short imageWidth;
    unsigned short imageHeight;
    LocalColorTableFlag localColorTable;
} ImageDescriptor;

typedef struct UserInputFlag
{
    unsigned int reserved;
    unsigned int method;
    unsigned int i;
    unsigned int t;
} UserInputFlag;

typedef struct ApplicationData
{
    unsigned char size;
    unsigned char data[256];   // 256 bytes
} ApplicationData;

typedef struct ExtensionIntroducer
{
    unsigned char magic;
    unsigned char ExtensionLabel;
    unsigned char BlockSize;

    // 0xF9
    UserInputFlag userInputFlag;
    unsigned short DelayTime;
    unsigned char TransparentColorIndex;

    // 0xFF
    unsigned char ApplicationIdentifier[8]; // 8 bytes
    unsigned char AuthenticationCode[3];  // 3 bytes
    ApplicationData applicationData;  // Data Sub-blocks

    unsigned char BlockTerminator; // 1 byte
} ExtensionIntroducer;

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

static int ParserImageDescriptor(gif, bs)
{
    if (DBitStreamGetLeftSize(bs) < 10 - 1)
        return -1;

    ImageDescriptor id;
    id.magic = Magic_Image_Descriptor;

    DBitStreamReadShort(bs, &id.offsetX);
    DBitStreamReadShort(bs, &id.offsetY);
    DBitStreamReadShort(bs, &id.imageWidth);
    DBitStreamReadShort(bs, &id.imageHeight);

    unsigned char temp;
    DBitStreamReadChar(bs, &temp);
    id.localColorTable.m = (temp & 0x80) >> 7;
    id.localColorTable.i = (temp & 0x40) >> 6;
    id.localColorTable.s = (temp & 0x20) >> 5;
    id.localColorTable.r = (temp & 0x18) >> 3;
    id.localColorTable.pixel = (temp & 0x07) ;

    return 0;
}

static int ParserExtensionIntroducer(gif, bs)
{
    if (DBitStreamGetLeftSize(bs) < 8 - 1)
        return -1;

    ExtensionIntroducer ei;
    ei.magic = Magic_Extension_Introducer;

    DBitStreamReadChar(bs, &ei.ExtensionLabel);
    DBitStreamReadChar(bs, &ei.BlockSize);
    switch (ei.ExtensionLabel)
    {
    case 0xF9:
        {
            unsigned char temp;
            DBitStreamReadChar(bs, &temp);
            ei.userInputFlag.reserved = 0;
            ei.userInputFlag.method = (temp & 0x3C) >> 2;
            ei.userInputFlag.i = (temp & 0x02) >> 1;
            ei.userInputFlag.t = (temp & 0x01) >> 0;
            DBitStreamReadShort(bs, &ei.DelayTime);
            DBitStreamReadChar(bs, &ei.TransparentColorIndex);

            DBitStreamReadChar(bs, &ei.BlockTerminator);
        }
        break;
    case 0xFF:
        {
            DBitStreamReadBuf(bs, ei.ApplicationIdentifier, 8);
            DBitStreamReadBuf(bs, ei.AuthenticationCode, 3);
            DBitStreamReadChar(bs, &ei.applicationData.size);
            DBitStreamReadBuf(bs, ei.applicationData.data, ei.applicationData.size);

            DBitStreamReadChar(bs, &ei.BlockTerminator);
        }
        break;
    default:
        break;
    }

    return 0;
}

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

    while (DBitStreamGetLeftSize(bs) > 0)
    {
        unsigned char magic;
        DBitStreamReadChar(bs, &magic);

        switch (magic)
        {
        case Magic_Image_Descriptor:
            ParserImageDescriptor(gif, bs);
            break;
        case Magic_Extension_Introducer:
            ParserExtensionIntroducer(gif, bs);
            break;
        default:
            break;
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
