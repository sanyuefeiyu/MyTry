#include <stdlib.h>
#include "DLoad.h"
#include "DLog.h"
#include "DFFmpeg.h"

#define TAG     "DFFmpeg"

typedef struct
{
    void *hdlAVCodec;
    void *hdlAVFormat;
    void *hdlAVUtil;
    void *hdlSWResamble;
} DFFmpeg;

DEXPORT void* DFFmpegInit()
{
    DFFmpeg *hdlFFmpeg = calloc(1, sizeof(DFFmpeg));
    if (hdlFFmpeg == NULL)
    {
        return NULL;
    }

    hdlFFmpeg->hdlAVFormat = DLoadOpen("avformat-57.dll");
    hdlFFmpeg->hdlAVCodec = DLoadOpen("avcodec-57.dll");
    hdlFFmpeg->hdlAVUtil = DLoadOpen("avutil-55.dll");
    hdlFFmpeg->hdlSWResamble = DLoadOpen("swresample-2.dll");

    if (hdlFFmpeg->hdlAVFormat == NULL
        || hdlFFmpeg->hdlAVCodec == NULL
        || hdlFFmpeg->hdlAVUtil == NULL
        || hdlFFmpeg->hdlSWResamble == NULL)
    {
        DFFmpegRelease(&hdlFFmpeg);
        return NULL;
    }

    return hdlFFmpeg;
}

DEXPORT void DFFmpegRelease(void **hdl)
{
    if (hdl == NULL || *hdl == NULL)
    {
        return;
    }

    DFFmpeg *hdlFFmpeg = *hdl;

    if (hdlFFmpeg->hdlAVFormat != NULL)
    {
        DLoadClose(hdlFFmpeg->hdlAVFormat);
    }
    if (hdlFFmpeg->hdlAVCodec != NULL)
    {
        DLoadClose(hdlFFmpeg->hdlAVCodec);
    }
    if (hdlFFmpeg->hdlAVUtil != NULL)
    {
        DLoadClose(hdlFFmpeg->hdlAVUtil);
    }
    if (hdlFFmpeg->hdlSWResamble != NULL)
    {
        DLoadClose(hdlFFmpeg->hdlSWResamble);
    }

    free(hdlFFmpeg);
    *hdl = NULL;
}
