#include <stdlib.h>
#include <stdio.h>
#include <DFile.h>
#include "DLog.h"
#include "GlobalConfig.h"

extern const char *gFilePath;
extern const char *gPCMOutputPath;

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

    FILE *fp = fopen(gFilePath, "rb+");
    fseek(fp, 0, SEEK_END);
    sourceLen = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    sourceData = malloc(sourceLen);
    fread(sourceData, sourceLen, 1, fp);
    fclose(fp);

    // open decoder 

    size_t pos = 0;
    const unsigned int BUFFER_SIZE = 1792;

    void *hdlFFmpegDDP = NULL;
    HA_LIBFFmpegDDPDecInit((void**)&hdlFFmpegDDP, NULL);

    while (pos + BUFFER_SIZE <= sourceLen)
    {
        // decode bitrate
        HA_LIBFFmpegDDPDecDecodeFrame(hdlFFmpegDDP, sourceData + pos, BUFFER_SIZE);

        WritePCM(hdlFFmpegDDP, gPCMOutputPath);

        pos += BUFFER_SIZE;
    }

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
