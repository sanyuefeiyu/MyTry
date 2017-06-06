#include <stdlib.h>
#include <string.h>
#include "DBitStream.h"
#include "DLog.h"
#include "DParseEac3.h"

#define TAG     "ParseEac3"

static unsigned char SYNC_WORD[2] = { 0x0B, 0x77 };
static const int SampleRateCodes[4] = { 48000, 44100, 32000, 0 };
static const int FrameSizeCodeTable[38][4] = { {64, 69, 96, 0}, {64, 70, 96, 0}, {80, 87, 120, 0}, {80, 88, 120, 0},
                                                {96, 104, 144, 0}, {96, 105, 144, 0}, {112, 121, 168, 0}, {112, 122, 168, 0},
                                                {128, 139, 192, 0}, {128, 140, 192, 0}, {160, 174, 240, 0}, {160, 175, 240, 0},
                                                {192, 208, 288, 0}, {192, 209, 288, 0}, {224, 243, 336, 0}, {224, 244, 336, 0},
                                                {256, 278, 384, 0}, {256, 279, 384, 0}, {320, 348, 480, 0}, {320, 349, 480, 0},
                                                {384, 417, 576, 0}, {384, 418, 576, 0}, {448, 487, 672, 0}, {448, 488, 672, 0},
                                                {512, 557, 768, 0}, {512, 558, 768, 0}, {640, 696, 960, 0}, {640, 697, 960, 0},
                                                {768, 835, 1152, 0}, {768, 836, 1152, 0}, {896, 975, 1344, 0}, {896, 976, 1344, 0},
                                                {1024, 1114, 1536, 0}, {1024, 1115, 1536, 0}, {1152, 1253, 1728, 0}, {1152, 1254, 1728, 0},
                                                {1280, 1393, 1920, 0}, {1280, 1394, 1920, 0} };
static const int BitRateTable[38] = { 32000, 32000, 40000, 40000, 48000, 48000,
                                        56000, 56000, 64000, 64000, 80000, 80000, 96000,
                                        96000, 112000, 112000, 128000, 128000, 160000, 160000,
                                        192000, 192000, 224000, 224000, 256000, 256000, 320000,
                                        320000, 384000, 384000, 448000, 448000, 512000, 512000,
                                        576000, 576000, 640000, 640000 };

DEXPORT int DParseEac3(const unsigned char *buf, int size, SyncFrame *frame)
{
    if (buf == NULL || size <= 0 || frame == NULL)
        return -1;

    void *bs = DBitStreamInit((unsigned char*)buf, size);
    if (bs == NULL)
        return -1;

    bool found = false;
    unsigned char syncword[2];
    unsigned char ch;
    do
    {
        if (DBitStreamGetLeftSize(bs) <= 6)
            break;

        // find 0x0B
        if (DBitStreamReadBuf(bs, syncword, 1) < 0)
            break;
        if (syncword[0] != 0x0B)
            continue;

        // find 0x77
        if (DBitStreamReadBuf(bs, syncword + 1, 1) < 0)
            break;
        if (syncword[1] != 0x77)
            continue;

        // DLog(DLOG_D, TAG, "find syncwod at %d pos", DBitStreamGetPos(bs) - 2);

        // AC-3

        // get crc
        if (DBitStreamReadBuf(bs, (unsigned char*)&frame->syncInfo.crc1, 2) < 0)
            break;
        if (DBitStreamReadBuf(bs, &ch, 1) < 0)
            break;

        // get fscod and frmsizecod
        frame->syncInfo.fscod = (ch & 0xC0) >> 6;
        if (frame->syncInfo.fscod == 0x03)
            continue;
        frame->syncInfo.frmsizecod = ch & 0x3F;
        if (frame->syncInfo.frmsizecod >= 38)
            continue;
        frame->sampleRate = SampleRateCodes[frame->syncInfo.fscod];

        frame->frameSize = FrameSizeCodeTable[frame->syncInfo.frmsizecod][frame->syncInfo.fscod] * 2;
        frame->bitrate = BitRateTable[frame->syncInfo.frmsizecod];
        DLog(DLOG_D, TAG, "get bitrate attr is %d, %d, %d", frame->sampleRate, frame->frameSize, frame->bitrate);

        // no enough buffer
        if (DBitStreamGetLeftSize(bs) + 5 < frame->frameSize)
            break;

        found = true;
        frame->startPos = DBitStreamGetPos(bs) - 5;
        break;
    } while (1);

    DBitStreamRelease(&bs);

    if (found)
        return 0;

    return -1;
}
