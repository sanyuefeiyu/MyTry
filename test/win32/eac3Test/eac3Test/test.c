#include <stdio.h>
#include "DFile.h"
#include "DLog.h"
#include "GlobalConfig.h"

#define inline __inline
#include "libavformat\avformat.h"
#include "libswresample\swresample.h"

#define DEFAULT_CHANNELS        2
#define DEFAULT_SAMPLE_BITS     16

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
    int ret = av_strerror(err, errbuf, 1024);
    DLog(DLOG_E, TAG, "PrintErrMsg:%s", errbuf);
}

static void SwrRelease(VideoState *vs)
{
    if (vs->swrInit)
    {
        swr_free(&vs->swr);
        vs->swrInit = 0;
    }
}

static void SwrInit(VideoState *vs)
{
    if (vs->swrChannelLayout != vs->channelLayout)
        SwrRelease(vs);

    if (vs->swrInit)
        return;

    vs->swr = swr_alloc();
    vs->swrChannelLayout = vs->channelLayout;
    av_opt_set_int(vs->swr, "in_channel_layout", vs->swrChannelLayout, 0);
    av_opt_set_int(vs->swr, "out_channel_layout", AV_CH_LAYOUT_STEREO, 0);
    av_opt_set_sample_fmt(vs->swr, "in_sample_fmt", vs->sampleFormat, 0);
    av_opt_set_sample_fmt(vs->swr, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);

    swr_init(vs->swr);
    vs->swrInit = 1;
}

static void AudioPostProcess(VideoState *vs, uint8_t *buff[])
{
    uint8_t *output;

    SwrInit(vs);

    av_samples_alloc(&output, NULL, DEFAULT_CHANNELS, vs->samples, AV_SAMPLE_FMT_S16, 0);
    swr_convert(vs->swr, &output, vs->samples, (const uint8_t**)buff, vs->samples);

    DFileWrite2Dest(gPCMOutputPath, output, DEFAULT_CHANNELS * DEFAULT_SAMPLE_BITS / 8 * vs->samples);

    av_freep(&output);
}

static void InitFFmpeg()
{
    av_register_all();
}

int decode_interrupt_cb(void *ctx)
{
    DLog(DLOG_D, TAG, "decode_interrupt_cb");
    return 0;
}

static int open_decoder(VideoState *vs, int stream_index)
{
    AVCodecContext *avctx = avcodec_alloc_context3(NULL);
    if (!avctx)
        return AVERROR(ENOMEM);
    vs->avctx = avctx;

    AVFormatContext *ic = vs->ic;
    int ret = avcodec_parameters_to_context(avctx, ic->streams[stream_index]->codecpar);
    if (ret < 0)
    {
        PrintErrMsg(ret);
        return AVERROR(ret);
    }

    av_codec_set_pkt_timebase(avctx, ic->streams[stream_index]->time_base);
    AVCodec *codec = avcodec_find_decoder(avctx->codec_id);
    if (!codec)
    {
        PrintErrMsg(ret);
        return AVERROR(ret);
    }
    avctx->codec_id = codec->id;

    int stream_lowres = av_codec_get_max_lowres(codec);
    ret = avcodec_open2(avctx, codec, NULL);
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
        int ret = avcodec_receive_frame(vs->avctx, frame);

        if (ret == AVERROR(EAGAIN))
        {
            break;
        }
        else if (ret == 0)
        {
            DLog(DLOG_D, TAG, "get output data=%lld, %d, %d, %d, %d, %d, %lld",
                                frame->pts,
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
        int ret = avcodec_send_packet(vs->avctx, pkt);
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
    InitFFmpeg();

    AVFormatContext *ic = avformat_alloc_context();
    ic->interrupt_callback.callback = decode_interrupt_cb;
    ic->interrupt_callback.opaque = NULL;
    vs.ic = ic;

    int ret = avformat_open_input(&ic, gFilePath, NULL, NULL);
    if (ret < 0)
    {
        PrintErrMsg(ret);
        return;
    }

    av_format_inject_global_side_data(ic);

    int orig_nb_streams = ic->nb_streams;
    int st_index[AVMEDIA_TYPE_NB];
    char* wanted_stream_spec[AVMEDIA_TYPE_NB] = {0};
    memset(st_index, -1, sizeof(st_index));

    ret = av_find_best_stream(ic, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
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

    vs.frame = av_frame_alloc();
    AVPacket pkt1, *pkt = &pkt1;
    do
    {
        ret = av_read_frame(ic, pkt);
        if (ret < 0)
        {
            PrintErrMsg(ret);
            break;
        }
        DLog(DLOG_D, TAG, "read bytes:%d, %lld", pkt->size, pkt->pts);

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

    av_frame_free(&vs.frame);
}

static void TestInit()
{
    DLogFlush();
    DFileFlush(gPCMOutputPath);
}

static void TestRelease()
{
}

void TestDecoder()
{
    DLog(DLOG_D, TAG, "Test begin");

    TestInit();
    TestAudio();
    TestRelease();

    DLog(DLOG_D, TAG, "Test end");
}
