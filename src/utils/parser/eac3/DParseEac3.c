#include <stdlib.h>
#include <string.h>
#include "DBitStream.h"
#include "DLog.h"
#include "DParseEac3.h"

#define TAG     "ParseEac3"

static unsigned char SYNC_WORD[2] = { 0x0B, 0x77 };
static int SampleRateCodes[4] = { 48000, 44100, 32000, -1 };

typedef struct
{
    unsigned short syncword; // 16
    unsigned short crc1; // 16
    unsigned char fscod; // 2
    unsigned char frmsizecod; // 6
} SyncInfo;

typedef struct
{
    SyncInfo syncInfo;

    int sampleRate;
    int frameSize;
} SyncFrame;

DEXPORT void DParseEac3(const unsigned char *buf, int size)
{
    if (buf == NULL || size <= 0)
        return;

    void *bs = DBitStreamInit((unsigned char*)buf, size);
    if (bs == NULL)
        return;

    SyncFrame frame;

    unsigned char syncword[2];
    unsigned char ch;
    do
    {
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

        DLog(DLOG_D, TAG, "find syncwod at %d pos", DBitStreamGetPos(bs));

        if (DBitStreamReadBuf(bs, (unsigned char*)&frame.syncInfo.crc1, 2) < 0)
            break;

        if (DBitStreamReadBuf(bs, &ch, 1) < 0)
            break;

        frame.syncInfo.fscod = (ch & 0xC0) >> 6;
        frame.syncInfo.frmsizecod = ch & 0x3F;
        frame.sampleRate = SampleRateCodes[frame.syncInfo.fscod];
        frame.frameSize = 1 << frame.syncInfo.frmsizecod;

    } while (1);

}
