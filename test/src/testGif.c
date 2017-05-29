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

typedef struct
{
    unsigned char alpha;
    unsigned char r;
    unsigned char g;
    unsigned char b;
} DRGB;

typedef struct
{
    unsigned short logicalScreenWidth;
    unsigned short logicalScreenHeight;

    unsigned int globalColorTableFlag;
    unsigned int colorResolution;
    unsigned int sortFlag;
    unsigned int pixel;

    unsigned char backgroundColor;
    unsigned char pixelAspectRatio;

    DRGB globalColorTable[256];
} LogicalScreenDescriptor;

typedef struct
{
    unsigned int m;
    unsigned int i;
    unsigned int s;
    unsigned int r;
    unsigned int pixel;
} LocalColorTableFlag;

typedef struct
{
    unsigned char magic;
    unsigned short offsetX;
    unsigned short offsetY;
    unsigned short imageWidth;
    unsigned short imageHeight;
    LocalColorTableFlag localColorTable;
} ImageDescriptor;

typedef struct
{
    unsigned char magic;
    unsigned char Label;
    unsigned char BlockSize;
} BlockHeader;

typedef struct
{
    unsigned int reserved;
    unsigned int method;
    unsigned int i;
    unsigned int t;
} UserInputFlag;

typedef struct
{
    unsigned char size;
    unsigned char data[256];   // 256 bytes
} ApplicationData;

typedef struct
{
    BlockHeader header;

    // 0xF9(Graphic Control Extension)
    UserInputFlag userInputFlag;
    unsigned short DelayTime;
    unsigned char TransparentColorIndex;

    // 0xFF(Application Extension)
    unsigned char ApplicationIdentifier[8]; // 8 bytes
    unsigned char AuthenticationCode[3];  // 3 bytes
    ApplicationData applicationData;  // Data Sub-blocks

    unsigned char BlockTerminator; // 1 byte
} ExtensionIntroducer;

#define GIF_HEADER_87A  "GIF87a"    // 1987年5月
#define GIF_HEADER_89A  "GIF89a"    // 1989年7月
typedef struct
{
    unsigned char version[32];

    LogicalScreenDescriptor lsd;
} DGif;

static int ParseHeader(DGif *gif, void *bs)
{
    if (DBitStreamReadBuf(bs, gif->version, strlen(GIF_HEADER_87A)) < 0)
    {
        return -1;
    }

    if (memcmp(gif->version, GIF_HEADER_87A, strlen(GIF_HEADER_87A)) != 0
        && memcmp(gif->version, GIF_HEADER_89A, strlen(GIF_HEADER_89A)) != 0)
        return -1;

    return 0;
}

static int ParseLogicalScreenDescriptor(DGif *gif, void *bs)
{
    // get width and height
    if (DBitStreamReadShort(bs, &gif->lsd.logicalScreenWidth) < 0)
        return -1;
    if (DBitStreamReadShort(bs, &gif->lsd.logicalScreenHeight) < 0)
        return -1;

    unsigned char ch;
    if (DBitStreamReadChar(bs, &ch) < 0)
        return -1;
    gif->lsd.globalColorTableFlag = (ch & 0x80) >> 7;
    gif->lsd.colorResolution = (ch & 0x70) >> 4;
    gif->lsd.sortFlag = (ch & 0x08) >> 3;
    gif->lsd.pixel = (ch & 0x07);

    if (DBitStreamReadChar(bs, &gif->lsd.backgroundColor) < 0)
        return -1;
    if (DBitStreamReadChar(bs, &gif->lsd.pixelAspectRatio) < 0)
        return -1;

    if (gif->lsd.globalColorTableFlag)
    {
        int pixelSize = 1 << (gif->lsd.pixel + 1);
        if (DBitStreamGetLeftSize(bs) < pixelSize * 3)
            return -1;

        for (int i = 0; i < pixelSize; i++)
        {
            unsigned char rgb[3];
            DBitStreamReadBuf(bs, rgb, 3);
            gif->lsd.globalColorTable[i].r = rgb[0];
            gif->lsd.globalColorTable[i].g = rgb[1];
            gif->lsd.globalColorTable[i].b = rgb[2];
        }
    }

    return 0;
}

static int ParserImageDescriptor(DGif *gif, void *bs)
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

static int ParserExtensionIntroducer(DGif *gif, void *bs)
{
    if (DBitStreamGetLeftSize(bs) < 8 - 1)
        return -1;

    ExtensionIntroducer ei;
    ei.header.magic = Magic_Extension_Introducer;
    DBitStreamReadChar(bs, &ei.header.Label);
    DBitStreamReadChar(bs, &ei.header.BlockSize);

    switch (ei.header.Label)
    {
    case 0xF9:
        {
            unsigned char ch;
            DBitStreamReadChar(bs, &ch);
            ei.userInputFlag.reserved = 0;
            ei.userInputFlag.method = (ch & 0x3C) >> 2;
            ei.userInputFlag.i = (ch & 0x02) >> 1;
            ei.userInputFlag.t = (ch & 0x01) >> 0;
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
        return -1;
    }

    return 0;
}

static DGif* ParseGif(const char *buf, const int size)
{
    // check inputs
    if (buf == NULL || size <= 0)
        return NULL;

    // alloc memory
    DGif *gif = calloc(1, sizeof(DGif));
    if (gif == NULL)
        return NULL;

    // create bitstream
    void *bs = DBitStreamInit((unsigned char*)buf, size);
    if (bs == NULL)
    {
        free(gif);
        return NULL;
    }

    // parse header
    if (ParseHeader(gif, bs) != 0)
        goto fail;

    // parse logical window
    if (ParseLogicalScreenDescriptor(gif, bs) != 0)
        goto fail;

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
            goto fail;
        }
    }

    free(bs);
    return gif;

fail:
    free(bs);
    free(gif);
    return NULL;
}

const char *filePath0 = "D:\\study\\CS\\res\\gif\\0.gif";

static void TestParseGif()
{
    // read data from file

    unsigned char *data = NULL;
    size_t size = 0;

    FILE *fp = fopen(filePath0, "rb+");
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    data = malloc(size);
    fread(data, size, 1, fp);
    fclose(fp);

    // start to parse Gif

    DGif *gif = ParseGif(data, size);
    if (gif != NULL)
    {
        free(gif);
    }

    free(data);
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
