#include <stdio.h>
#include "DFile.h"
#include "DLoad.h"
#include "DFFmpeg.h"
#include "DLog.h"
#include "GlobalConfig.h"

#define inline __inline
#include "libavformat\avformat.h"
#include "libswresample\swresample.h"

#define TAG "eac3Test"

const char *gFilePath = FILE_PATH_13;
const char *gPCMOutputPath = "d:\\audio.pcm";

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

    DFileWrite(gPCMOutputPath, output, DEFAULT_CHANNELS * DEFAULT_SAMPLE_BITS / 8 * vs->samples);

    DFFmpeg_av_freep(gHdlFFmpeg, &output);
}

static void InitFFmpeg()
{
    DFFmpeg_av_register_all(gHdlFFmpeg);
}

int decode_interrupt_cb(void *ctx)
{
    DLog(DLOG_D, TAG, "decode_interrupt_cb");
    return 0;
}

static int open_decoder(VideoState *vs, int stream_index)
{
    AVCodecContext *avctx = DFFmpeg_avcodec_alloc_context3(gHdlFFmpeg, NULL);
    if (!avctx)
        return AVERROR(ENOMEM);
    vs->avctx = avctx;

    AVFormatContext *ic = vs->ic;
    int ret = DFFmpeg_avcodec_parameters_to_context(gHdlFFmpeg, avctx, ic->streams[stream_index]->codecpar);
    if (ret < 0)
    {
        PrintErrMsg(ret);
        return AVERROR(ret);
    }

    DFFmpeg_av_codec_set_pkt_timebase(gHdlFFmpeg, avctx, ic->streams[stream_index]->time_base);
    AVCodec *codec = DFFmpeg_avcodec_find_decoder(gHdlFFmpeg, avctx->codec_id);
    if (!codec)
    {
        PrintErrMsg(ret);
        return AVERROR(ret);
    }
    avctx->codec_id = codec->id;

    int stream_lowres = DFFmpeg_av_codec_get_max_lowres(gHdlFFmpeg, codec);
    ret = DFFmpeg_avcodec_open2(gHdlFFmpeg, avctx, codec, NULL);
    if (ret < 0)
    {
        PrintErrMsg(ret);
        return AVERROR(ret);
    }

    AVERROR(ret);
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

    InitFFmpeg();

    AVFormatContext *ic = DFFmpeg_avformat_alloc_context(gHdlFFmpeg);
    ic->interrupt_callback.callback = decode_interrupt_cb;
    ic->interrupt_callback.opaque = NULL;
    vs.ic = ic;

    int ret = DFFmpeg_avformat_open_input(gHdlFFmpeg, &ic, gFilePath, NULL, NULL);
    if (ret < 0)
    {
        PrintErrMsg(ret);
        return;
    }

    DFFmpeg_av_format_inject_global_side_data(gHdlFFmpeg, ic);

    int orig_nb_streams = ic->nb_streams;
    int st_index[AVMEDIA_TYPE_NB];
    char* wanted_stream_spec[AVMEDIA_TYPE_NB] = {0};
    memset(st_index, -1, sizeof(st_index));

    ret = DFFmpeg_av_find_best_stream(gHdlFFmpeg, ic, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if (ret < 0)
    {
        PrintErrMsg(ret );
        // return;
    }
    // st_index[AVMEDIA_TYPE_AUDIO] = ret;
    st_index[AVMEDIA_TYPE_AUDIO] = 0;

    if (st_index[AVMEDIA_TYPE_AUDIO] >= 0)
    {
        open_decoder(&vs, st_index[AVMEDIA_TYPE_AUDIO]);
    }

    vs.frame = DFFmpeg_av_frame_alloc(gHdlFFmpeg);
    AVPacket pkt1, *pkt = &pkt1;
    AVRational tb = vs.ic->streams[0]->time_base;
    do
    {
        ret = DFFmpeg_av_read_frame(gHdlFFmpeg, ic, pkt);
        if (ret < 0)
        {
            PrintErrMsg(ret);
            break;
        }
        DLog(DLOG_D, TAG, "read bytes:%d, %lld", pkt->size, 1000 * pkt->pts * tb.num / tb.den);

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
    } while (1);

    DFFmpeg_av_frame_free(gHdlFFmpeg, &vs.frame);
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

void TestDecoder()
{
    DLog(DLOG_D, TAG, "Test begin");

    TestInit();

    TestAudio();

    TestRelease();

    DLog(DLOG_D, TAG, "Test end");
}
