#include <stdlib.h>
#include <stdio.h>
#include <DFile.h>
#include "DParseEac3.h"
#include "DLog.h"
#include "GlobalConfig.h"

#define TAG     "Test2"

int HA_LIBFFmpegDDPDecInit(void **phDecoder, const void *pstOpenParam);
int HA_LIBFFmpegDDPDecDeInit(void *hDecoder);
int HA_LIBFFmpegDDPDecDecodeFrame(void *hDecoder, unsigned char *buff, unsigned int size);
void WritePCM(void *hDecoder, const char *path);

static void TestAudio()
{
    // read data from file

    unsigned char *sourceData = NULL;
    size_t sourceLen = 0;

    FILE *fp = fopen(g_FILE_PATH, "rb+");
    fseek(fp, 0, SEEK_END);
    sourceLen = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    sourceData = malloc(sourceLen);
    fread(sourceData, sourceLen, 1, fp);
    fclose(fp);

    // open decoder

    void *hdlFFmpegDDP = NULL;
    HA_LIBFFmpegDDPDecInit((void**)&hdlFFmpegDDP, NULL);

    size_t pos = 0;
    do
    {
        SyncFrame frame;
        if (DParseEac3(sourceData + pos, sourceLen - pos, &frame) != 0)
            break;

        // decode bitrate
        if (HA_LIBFFmpegDDPDecDecodeFrame(hdlFFmpegDDP, sourceData + pos + frame.startPos, frame.frameSize) == 0)
        {
            WritePCM(hdlFFmpegDDP, gPCMOutputPath);
        }
        else
        {
            DLog(DLOG_D, TAG, "decode failed!!!!!!");
        }

        pos += (frame.frameSize + frame.startPos);
    } while (1);

    // close decoder
    HA_LIBFFmpegDDPDecDeInit(hdlFFmpegDDP);

    free(sourceData);
}

void TestDecoder2()
{
    // first clear old data
    DLogFlush();
    DFileFlush(gPCMOutputPath);

    DLog(DLOG_D, TAG, "Test begin");

    // test eac3 FFmpeg decoder
    TestAudio();

    DLog(DLOG_D, TAG, "Test end");
}
