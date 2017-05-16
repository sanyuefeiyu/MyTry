#include <stdio.h>
#include "DFile.h"
#include "DLoad.h"
#include "DFFmpeg.h"
#include "DLog.h"
#include "GlobalConfig.h"

#define inline __inline
#include "libavformat\avformat.h"
#include "libswresample\swresample.h"

extern const char *gFilePath;
extern const char *gPCMOutputPath;

#define DEFAULT_CHANNELS        2
#define DEFAULT_SAMPLE_BITS     16

static void *gHdlFFmpeg = NULL;

typedef struct VideoState
{
    AVFormatContext *ic;
    AVCodecContext *avctx;
    AVFrame *frame;

    unsigned long long channelLayout;
    unsigned int sampleRate;
    unsigned int sampleFormat;
    unsigned int samples;

    SwrContext *swr;
    unsigned int swrInit;
    unsigned long long swrChannelLayout;
} VideoState;

static VideoState vs;

static void PrintErrMsg(int err)
{
    char errbuf[1024];
    int ret = DFFmpeg_av_strerror(gHdlFFmpeg, err, errbuf, 1024);
    DLog(DLOG_E, TAG, "PrintErrMsg:%s", errbuf);
}

static void SwrRelease(VideoState *vs)
{
    if (vs->swrInit)
    {
        DFFmpeg_swr_free(gHdlFFmpeg, &vs->swr);
        vs->swrInit = 0;
    }
}

static void SwrInit(VideoState *vs)
{
    if (vs->swrChannelLayout != vs->channelLayout)
        SwrRelease(vs);

    if (vs->swrInit)
        return;

    vs->swr = DFFmpeg_swr_alloc(gHdlFFmpeg);
    vs->swrChannelLayout = vs->channelLayout;
    DFFmpeg_av_opt_set_int(gHdlFFmpeg, vs->swr, "in_channel_layout", vs->swrChannelLayout, 0);
    DFFmpeg_av_opt_set_int(gHdlFFmpeg, vs->swr, "out_channel_layout", AV_CH_LAYOUT_STEREO, 0);
    DFFmpeg_av_opt_set_sample_fmt(gHdlFFmpeg, vs->swr, "in_sample_fmt", vs->sampleFormat, 0);
    DFFmpeg_av_opt_set_sample_fmt(gHdlFFmpeg, vs->swr, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);

    DFFmpeg_swr_init(gHdlFFmpeg, vs->swr);
    vs->swrInit = 1;
}

static void AudioPostProcess(VideoState *vs, uint8_t *buff[])
{
    uint8_t *output;

    SwrInit(vs);

    DFFmpeg_av_samples_alloc(gHdlFFmpeg, &output, NULL, DEFAULT_CHANNELS, vs->samples, AV_SAMPLE_FMT_S16, 0);
    DFFmpeg_swr_convert(gHdlFFmpeg, vs->swr, &output, vs->samples, (const uint8_t**)buff, vs->samples);

    DFileWrite2Dest(gPCMOutputPath, output, DEFAULT_CHANNELS * DEFAULT_SAMPLE_BITS / 8 * vs->samples);

    DFFmpeg_av_freep(gHdlFFmpeg, &output);
}

static int decode_interrupt_cb(void *ctx)
{
    DLog(DLOG_D, TAG, "decode_interrupt_cb");
    return 0;
}

static int open_decoder(VideoState *vs)
{
    DFFmpeg_av_register_all(gHdlFFmpeg);

    AVCodec *codec = DFFmpeg_avcodec_find_decoder_by_name(gHdlFFmpeg, "eac3");
    if (!codec)
    {
        return AVERROR(EINVAL);
    }

    AVCodecContext *avctx = DFFmpeg_avcodec_alloc_context3(gHdlFFmpeg, codec);
    if (!avctx)
        return AVERROR(ENOMEM);
    vs->avctx = avctx;

    int ret = DFFmpeg_avcodec_open2(gHdlFFmpeg, avctx, codec, NULL);
    if (ret < 0)
    {
        PrintErrMsg(ret);
        return AVERROR(ret);
    }

    return AVERROR(ret);
}

static int GetDecodeOutput(VideoState *vs, AVFrame *frame)
{
    do
    {
        int ret = DFFmpeg_avcodec_receive_frame(gHdlFFmpeg, vs->avctx, frame);

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

            vs->channelLayout = frame->channel_layout;
            vs->sampleRate = frame->sample_rate;
            vs->sampleFormat = frame->format;
            vs->samples = frame->nb_samples;

            AudioPostProcess(vs, frame->data);
        }
        else
        {
            PrintErrMsg(ret);
            return -1;
        }
    } while (1);

    return 0;
}

static int Send2Decode(VideoState *vs, AVPacket *pkt)
{
    do
    {
        int ret = DFFmpeg_avcodec_send_packet(gHdlFFmpeg, vs->avctx, pkt);
        if (ret == AVERROR(EAGAIN))
        {
            GetDecodeOutput(vs, vs->frame);
            continue;
        }
        else if (ret == 0)
        {
            break;
        }
        else if (ret < 0)
        {
            PrintErrMsg(ret);
            break;
        }
    } while (1);

    return 0;
}

static void TestAudio()
{
    if (gHdlFFmpeg == NULL)
    {
        return;
    }

    open_decoder(&vs);
    vs.frame = DFFmpeg_av_frame_alloc(gHdlFFmpeg);

    unsigned char *sourceData = NULL;
    size_t sourceLen = 0;

    FILE *fp = fopen(gFilePath, "rb+");
    fseek(fp, 0, SEEK_END);
    sourceLen = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    sourceData = malloc(sourceLen);
    fread(sourceData, sourceLen, 1, fp);
    fclose(fp);

    size_t pos = 0;
    const unsigned int BUFFER_SIZE = 1792;
    AVPacket pkt1, *pkt = &pkt1;
    memset(pkt, 0, sizeof(AVPacket));
    pkt->data = malloc(BUFFER_SIZE);

    while (pos + BUFFER_SIZE <= sourceLen)
    {
        DLog(DLOG_D, TAG, "pos is [%d|%d]", pos, sourceLen);

        memcpy(pkt->data, sourceData + pos, BUFFER_SIZE);
        pkt->size = BUFFER_SIZE;

        // send to decode
        if (Send2Decode(&vs, pkt) < 0)
        {
            break;
        }

        // get decode output
        if (GetDecodeOutput(&vs, vs.frame) < 0)
        {
            break;
        }

        pos += BUFFER_SIZE;
    }

    DFFmpeg_av_frame_free(gHdlFFmpeg, &vs.frame);
    free(sourceData);
}

static void TestInit()
{
    // clear log
    DLogFlush();
    DFileFlush(gPCMOutputPath);

    // load FFmpeg libraries
    gHdlFFmpeg = DFFmpegInit();
}

static void TestRelease()
{
    // deload FFmpeg libraries
    DFFmpegRelease(&gHdlFFmpeg);
}

void TestDecoder2()
{
    DLog(DLOG_D, TAG, "Test begin");

    TestInit();

    TestAudio();

    TestRelease();

    DLog(DLOG_D, TAG, "Test end");
}
