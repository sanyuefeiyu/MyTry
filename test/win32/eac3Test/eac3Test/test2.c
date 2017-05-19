#include <stdio.h>
#include "DFile.h"
#include "DLoad.h"
#include "DFFmpeg.h"
#include "DLog.h"
#include "GlobalConfig.h"

#define inline __inline
#include "libswresample\swresample.h"

extern const char *gFilePath;
extern const char *gPCMOutputPath;

#define DEFAULT_CHANNELS        2
#define DEFAULT_SAMPLE_BITS     16

#define TAG     "LibFFmpegDDP"

typedef struct PCM
{
    unsigned char *data;
    unsigned int capacity;
    unsigned int size;
} PCM;

#define MAX_PCM_SIZE    (10*1024*1024)
static void PCMAdd(PCM *pcm, unsigned char *buff, unsigned int size)
{
    if (pcm == NULL || buff == NULL || size <=0)
    {
        return;
    }

    // init PCM buffer
    if (pcm->data == NULL)
    {
        pcm->data = malloc(size);
        if (pcm->data == NULL)
        {
            return;
        }
        pcm->capacity = size;
        pcm->size = 0;
    }

    // relocate PCM buffer
    if (size + pcm->size > pcm->capacity)
    {
        // reach at max presize, and do not malloc again
        if (pcm->capacity >= MAX_PCM_SIZE)
        {
            return;
        }

        unsigned char *tempBuff = malloc(size + pcm->size);
        if (tempBuff == NULL)
        {
            return;
        }

        memcpy(tempBuff, pcm->data, pcm->size);
        free(pcm->data);
        pcm->data = tempBuff;
        pcm->capacity = size + pcm->size;
    }

    // copy buffer to PCM
    memcpy(pcm->data + pcm->size, buff, size);
    pcm->size += size;
}

static void PCMReset(PCM *pcm)
{
    if (pcm == NULL)
    {
        return;
    }

    pcm->size = 0;
}

static void PCMClean(PCM *pcm)
{
    if (pcm == NULL)
    {
        return;
    }

    // clean buffer
    if (pcm->data != NULL)
    {
        free(pcm->data);
        pcm->data = NULL;
    }
    pcm->size = 0;
    pcm->capacity = 0;
}

typedef struct FFmpegDDP
{
    void *hdlFFmpeg;

    AVCodecContext *avctx;
    AVFrame *frame;

    unsigned long long channelLayout;
    unsigned int sampleRate;
    unsigned int sampleFormat;
    unsigned int samples;

    SwrContext *swr;
    unsigned int swrInit;
    unsigned long long swrChannelLayout;

    PCM pcm;
} FFmpegDDP;

static void PrintErrMsg(void *hdlFFmpeg, int err)
{
    char errbuf[1024];
    int ret = DFFmpeg_av_strerror(hdlFFmpeg, err, errbuf, 1024);
    DLog(DLOG_E, TAG, "PrintErrMsg:%s", errbuf);
}

static void SwrRelease(FFmpegDDP *hdlFFmpegDDP)
{
    if (hdlFFmpegDDP->swrInit)
    {
        DFFmpeg_swr_free(hdlFFmpegDDP->hdlFFmpeg, &hdlFFmpegDDP->swr);
        hdlFFmpegDDP->swrInit = 0;
    }
}

static void SwrInit(FFmpegDDP *hdlFFmpegDDP)
{
    if (hdlFFmpegDDP->swrChannelLayout != hdlFFmpegDDP->channelLayout)
        SwrRelease(hdlFFmpegDDP);

    if (hdlFFmpegDDP->swrInit)
        return;

    hdlFFmpegDDP->swr = DFFmpeg_swr_alloc(hdlFFmpegDDP->hdlFFmpeg);
    hdlFFmpegDDP->swrChannelLayout = hdlFFmpegDDP->channelLayout;
    SwrContext *swr = hdlFFmpegDDP->swr;
    DFFmpeg_av_opt_set_int(hdlFFmpegDDP->hdlFFmpeg,  swr, "in_channel_layout", hdlFFmpegDDP->swrChannelLayout, 0);
    DFFmpeg_av_opt_set_int(hdlFFmpegDDP->hdlFFmpeg,  swr, "out_channel_layout", AV_CH_LAYOUT_STEREO, 0);
    DFFmpeg_av_opt_set_sample_fmt(hdlFFmpegDDP->hdlFFmpeg, swr, "in_sample_fmt", hdlFFmpegDDP->sampleFormat, 0);
    DFFmpeg_av_opt_set_sample_fmt(hdlFFmpegDDP->hdlFFmpeg, swr, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);

    DFFmpeg_swr_init(hdlFFmpegDDP->hdlFFmpeg, swr);
    hdlFFmpegDDP->swrInit = 1;
}

static void AudioPostProcess(FFmpegDDP *hdlFFmpegDDP, uint8_t *buff[])
{
    uint8_t *output;

    SwrInit(hdlFFmpegDDP);

    DFFmpeg_av_samples_alloc(hdlFFmpegDDP->hdlFFmpeg, &output, NULL, DEFAULT_CHANNELS, hdlFFmpegDDP->samples, AV_SAMPLE_FMT_S16, 0);
    DFFmpeg_swr_convert(hdlFFmpegDDP->hdlFFmpeg, hdlFFmpegDDP->swr, &output, hdlFFmpegDDP->samples, (const uint8_t**)buff, hdlFFmpegDDP->samples);

    PCMAdd(&hdlFFmpegDDP->pcm, output, hdlFFmpegDDP->samples * DEFAULT_CHANNELS * DEFAULT_SAMPLE_BITS / 8);

    DFFmpeg_av_freep(hdlFFmpegDDP->hdlFFmpeg, &output);
}

static int GetDecodeOutput(FFmpegDDP *hdlFFmpegDDP, AVFrame *frame)
{
    do
    {
        int ret = DFFmpeg_avcodec_receive_frame(hdlFFmpegDDP->hdlFFmpeg, hdlFFmpegDDP->avctx, frame);

        if (ret == AVERROR(EAGAIN))
        {
            break;
        }
        else if (ret == 0)
        {
            DLog(DLOG_D, TAG, "get output data=%lld, %d, %d, %d, %d, %d, %lld",
                                frame->pts * 1000 / 90000,
                                frame->sample_rate,
                                frame->channels,
                                frame->nb_samples,
                                frame->linesize[0],
                                frame->format,
                                frame->channel_layout);

            hdlFFmpegDDP->channelLayout = frame->channel_layout;
            hdlFFmpegDDP->sampleRate = frame->sample_rate;
            hdlFFmpegDDP->sampleFormat = frame->format;
            hdlFFmpegDDP->samples = frame->nb_samples;

            AudioPostProcess(hdlFFmpegDDP, frame->data);
        }
        else
        {
            PrintErrMsg(hdlFFmpegDDP->hdlFFmpeg, ret);
            return -1;
        }
    } while (1);

    return 0;
}

static int Send2Decode(FFmpegDDP *hdlFFmpegDDP, AVPacket *pkt)
{
    do
    {
        int ret = DFFmpeg_avcodec_send_packet(hdlFFmpegDDP->hdlFFmpeg, hdlFFmpegDDP->avctx, pkt);
        if (ret == AVERROR(EAGAIN))
        {
            GetDecodeOutput(hdlFFmpegDDP, hdlFFmpegDDP->frame);
            continue;
        }
        else if (ret == 0)
        {
            break;
        }
        else if (ret < 0)
        {
            PrintErrMsg(hdlFFmpegDDP->hdlFFmpeg, ret);
            return -1;
        }
    } while (1);

    return 0;
}

static int open_decoder(FFmpegDDP *hdlFFmpegDDP)
{
    DFFmpeg_av_register_all(hdlFFmpegDDP->hdlFFmpeg);

    AVCodec *codec = DFFmpeg_avcodec_find_decoder_by_name(hdlFFmpegDDP->hdlFFmpeg, "eac3");
    if (!codec)
    {
        return AVERROR(EINVAL);
    }

    AVCodecContext *avctx = DFFmpeg_avcodec_alloc_context3(hdlFFmpegDDP->hdlFFmpeg, codec);
    if (!avctx)
        return AVERROR(ENOMEM);
    hdlFFmpegDDP->avctx = avctx;

    int ret = DFFmpeg_avcodec_open2(hdlFFmpegDDP->hdlFFmpeg, avctx, codec, NULL);
    if (ret < 0)
    {
        PrintErrMsg(hdlFFmpegDDP->hdlFFmpeg, ret);
        return AVERROR(ret);
    }

    return AVERROR(ret);
}

static int HA_LIBFFmpegDDPDecInit(void **phDecoder, const void *pstOpenParam)
{
    DLog(DLOG_D, TAG, "HA_LIBFFmpegDDPDecInit begin");

    FFmpegDDP *hdlFFmpegDDP = NULL;

    // if (!phDecoder || !pstOpenParam)
    if (!phDecoder)
    {
        DLog(DLOG_W, TAG, "invalid params");
        return -1;
    }

    // malloc memory
    hdlFFmpegDDP = (FFmpegDDP*)calloc(1, sizeof(FFmpegDDP));
    if (NULL == hdlFFmpegDDP)
    {
        DLog(DLOG_W, TAG, "malloc decoder failed");
        return -1;
    }

    // load FFmpeg libraries
    hdlFFmpegDDP->hdlFFmpeg = DFFmpegInit();
    if (hdlFFmpegDDP->hdlFFmpeg == NULL)
    {
        DLog(DLOG_W, TAG, "load FFmpeg libraries failed");
        free(hdlFFmpegDDP);
        return -1;
    }

    // open decoder
    open_decoder(hdlFFmpegDDP);

    // malloc AVFrame
    hdlFFmpegDDP->frame = DFFmpeg_av_frame_alloc(hdlFFmpegDDP->hdlFFmpeg);

    *phDecoder = (void*)hdlFFmpegDDP;

    return 0;
}

static int HA_LIBFFmpegDDPDecDeInit(void *hDecoder)
{
    FFmpegDDP *hdlFFmpegDDP = NULL;

    if (!hDecoder)
    {
        DLog(DLOG_W, TAG, "invalid params");
        return -1;
    }

    hdlFFmpegDDP = (FFmpegDDP*)hDecoder;

    // release PCM
    PCMClean(&hdlFFmpegDDP->pcm);

    // release SWR
    SwrRelease(hdlFFmpegDDP);

    // release AVFrame
    DFFmpeg_av_frame_free(hdlFFmpegDDP->hdlFFmpeg, &hdlFFmpegDDP->frame);

    // close decoder
    DFFmpeg_avcodec_close(hdlFFmpegDDP->hdlFFmpeg, hdlFFmpegDDP->avctx);

    // deload FFmpeg libraries
    DFFmpegRelease(&hdlFFmpegDDP->hdlFFmpeg);

    // release memory
    free(hdlFFmpegDDP);
    return 0;
}

static int HA_LIBFFmpegDDPDecDecodeFrame(void *hDecoder, unsigned char *buff, unsigned int size)
{
    FFmpegDDP *hdlFFmpegDDP = NULL;

    if (!hDecoder)
    {
        DLog(DLOG_W, TAG, "invalid params");
        return -1;
    }

    hdlFFmpegDDP = (FFmpegDDP*)hDecoder;
    PCMReset(&hdlFFmpegDDP->pcm);

    AVPacket pkt1, *pkt = &pkt1;
    memset(pkt, 0, sizeof(AVPacket));
    pkt->data = buff;
    pkt->size = size;

    // send to decode
    if (Send2Decode(hdlFFmpegDDP, pkt) < 0)
    {
        return -1;
    }

    // get decode output
    if (GetDecodeOutput(hdlFFmpegDDP, hdlFFmpegDDP->frame) < 0)
    {
        return -1;
    }

    return 0;
}

// HI_HA_DECODE_S ha_audio_decode_entry = {
//     .szName               = (const HI_PCHAR )"liba52 ac3 audio decoder ",
//     .enCodecID            = HA_AUDIO_ID_CUSTOM_0,//0x81f00055,//HA_AUDIO_ID_CUSTOM_0, //0x81f00400,  //HA_AUDIO_ID_AC3PASSTHROUGH,   //4.2  HA_AUDIO_ID_AC3PASSTHROUGH    4.4: 0x80020001   4.4 ali yun:0x81f01010
//     .uVersion.u32Version  =                                                   0x10000001,
//     .pszDescription       = (const HI_PCHAR)"ha referrence ac3 decoder based liba52",
//     .DecInit              = HA_LIBA52DecInit,
//     .DecDeInit            = HA_LIBA52DecDeInit,
//     .DecSetConfig         = HA_SetConfig,
//     .DecGetMaxPcmOutSize  = HA_LIBA52DecGetMaxPcmOutSize,
//     .DecGetMaxBitsOutSize = HA_LIBA52DecGetMaxBitsOutSize,
//     .DecDecodeFrame       = HA_LIBA52DecDecodeFrame,
// };

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

    FFmpegDDP *hdlFFmpegDDP = NULL;
    HA_LIBFFmpegDDPDecInit(&hdlFFmpegDDP, NULL);

    while (pos + BUFFER_SIZE <= sourceLen)
    {
        // decode bitrate
        HA_LIBFFmpegDDPDecDecodeFrame(hdlFFmpegDDP, sourceData + pos, BUFFER_SIZE);

        DFileWrite2Dest(gPCMOutputPath, hdlFFmpegDDP->pcm.data, hdlFFmpegDDP->pcm.size);

        pos += BUFFER_SIZE;
    }

    // close decoder
    HA_LIBFFmpegDDPDecDeInit(hdlFFmpegDDP);

    free(sourceData);
}

void TestDecoder2()
{
    DLog(DLOG_D, TAG, "Test begin");

    // first old data
    DLogFlush();
    DFileFlush(gPCMOutputPath);

    // test eac3 FFmpeg decoder
    TestAudio();

    DLog(DLOG_D, TAG, "Test end");
}
