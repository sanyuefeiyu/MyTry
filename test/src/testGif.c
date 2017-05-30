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
    unsigned char magic;
    unsigned short ImageLeftPosition;
    unsigned short ImageTopPosition;
    unsigned short ImageWidth;
    unsigned short ImageHeight;

    // <Packed Fields>
    unsigned int LocalColorTableFlag;   // 1 Bit. 0 - Local Color Table is not present. Use Global Color Table if available. 1 - Local Color Table present, and to follow immediately after this Image Descriptor.
    unsigned int InterlaceFlag; // 1 Bit. 0 - Image is not interlaced. 1 - Image is interlaced.
    unsigned int SortFlag; // 1 Bit
    unsigned int Reserved; // 2 Bits
    unsigned int SizeofLocalColorTable; // 3 Bits

	unsigned char LZWMinimumCodeSize;
} ImageDescriptor;

typedef struct
{
    unsigned char magic;
    unsigned char Label;
    unsigned char BlockSize;
} BlockHeader;

typedef struct
{
    unsigned char size;
    unsigned char data[256];   // 256 bytes
} ApplicationData;

typedef struct
{
    BlockHeader header;

    // 0xF9(Graphic Control Extension)
    // <Packed Fields>
    unsigned int Reserved; // 3 Bits
    unsigned int DisposalMethod; // 3 Bits
    unsigned int UserInputFlag; // 1 Bit
    unsigned int TransparentColorFlag; // 1 Bit

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

static int ParseImageDescriptor(DGif *gif, void *bs)
{
    if (DBitStreamGetLeftSize(bs) < 10 - 1)
        return -1;

    ImageDescriptor id;
    id.magic = Magic_Image_Descriptor;

    DBitStreamReadShort(bs, &id.ImageLeftPosition);
    DBitStreamReadShort(bs, &id.ImageTopPosition);
    DBitStreamReadShort(bs, &id.ImageWidth);
    DBitStreamReadShort(bs, &id.ImageHeight);

    unsigned char ch;
    DBitStreamReadChar(bs, &ch);
    id.LocalColorTableFlag = (ch & 0x80) >> 7;
    id.InterlaceFlag = (ch & 0x40) >> 6;
    id.SortFlag = (ch & 0x20) >> 5;
    id.Reserved = (ch & 0x18) >> 3;
    id.SizeofLocalColorTable = (ch & 0x07) ;

	DBitStreamReadChar(bs, &id.LZWMinimumCodeSize);
	unsigned char size;
	unsigned char data[256];
	DBitStreamReadChar(bs, &size);
	while (size != 0x00)
	{
		DBitStreamReadBuf(bs, data, size);
		DBitStreamReadChar(bs, &size);
	}

    return 0;
}

static int ParseExtensionIntroducer(DGif *gif, void *bs)
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
            ei.Reserved = 0;
            ei.DisposalMethod = (ch & 0x1C) >> 2;
            ei.UserInputFlag = (ch & 0x02) >> 1;
            ei.TransparentColorFlag = (ch & 0x01) >> 0;

            DBitStreamReadShort(bs, &ei.DelayTime);
            DBitStreamReadChar(bs, &ei.TransparentColorIndex);

            DBitStreamReadChar(bs, &ei.BlockTerminator);
        }
        break;
    case 0xFF:
        {
            if (DBitStreamReadBuf(bs, ei.ApplicationIdentifier, 8) != 0)
                return -1;
            if (DBitStreamReadBuf(bs, ei.AuthenticationCode, 3) != 0)
                return -1;
            if (DBitStreamReadChar(bs, &ei.applicationData.size) != 0)
                return -1;
            if (DBitStreamReadBuf(bs, ei.applicationData.data, ei.applicationData.size) != 0)
                return -1;

            if (DBitStreamReadChar(bs, &ei.BlockTerminator) != 0)
                return -1;
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

    if (ParseHeader(gif, bs) != 0)
        goto fail;

    if (ParseLogicalScreenDescriptor(gif, bs) != 0)
        goto fail;

    while (DBitStreamGetLeftSize(bs) > 0)
    {
        unsigned char magic;
        DBitStreamReadChar(bs, &magic);

        switch (magic)
        {
        case Magic_Image_Descriptor:
            if (ParseImageDescriptor(gif, bs) != 0)
                goto fail;
            break;
        case Magic_Extension_Introducer:
            if (ParseExtensionIntroducer(gif, bs) != 0)
                goto fail;
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
