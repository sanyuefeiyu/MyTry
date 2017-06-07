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


#define DEFAULT_CHANNELS        2
#define DEFAULT_SAMPLE_BITS     16

typedef struct FFmpegDDP
{
    void *hdlFFmpeg;
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
} FFmpegDDP;

static FFmpegDDP FFmpegDDPInstance;

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

    DFileWrite(gPCMOutputPath, output, hdlFFmpegDDP->samples * DEFAULT_CHANNELS * DEFAULT_SAMPLE_BITS / 8);

    DFFmpeg_av_freep(hdlFFmpegDDP->hdlFFmpeg, &output);
}

static void InitFFmpeg()
{
    DFFmpeg_av_register_all(FFmpegDDPInstance.hdlFFmpeg);
}

int decode_interrupt_cb(void *ctx)
{
    DLog(DLOG_D, TAG, "decode_interrupt_cb");
    return 0;
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

static int open_decoder(FFmpegDDP *hdlFFmpegDDP, int stream_index)
{
    AVCodecContext *avctx = DFFmpeg_avcodec_alloc_context3(hdlFFmpegDDP->hdlFFmpeg, NULL);
    if (!avctx)
        return AVERROR(ENOMEM);
    hdlFFmpegDDP->avctx = avctx;

    AVFormatContext *ic = hdlFFmpegDDP->ic;
    int ret = DFFmpeg_avcodec_parameters_to_context(hdlFFmpegDDP->hdlFFmpeg, avctx, ic->streams[stream_index]->codecpar);
    if (ret < 0)
    {
        PrintErrMsg(hdlFFmpegDDP->hdlFFmpeg, ret);
        return AVERROR(ret);
    }

    DFFmpeg_av_codec_set_pkt_timebase(hdlFFmpegDDP->hdlFFmpeg, avctx, ic->streams[stream_index]->time_base);
    AVCodec *codec = DFFmpeg_avcodec_find_decoder(hdlFFmpegDDP->hdlFFmpeg, avctx->codec_id);
    if (!codec)
    {
        PrintErrMsg(hdlFFmpegDDP->hdlFFmpeg, ret);
        return AVERROR(ret);
    }
    avctx->codec_id = codec->id;

    int stream_lowres = DFFmpeg_av_codec_get_max_lowres(hdlFFmpegDDP->hdlFFmpeg, codec);
    ret = DFFmpeg_avcodec_open2(hdlFFmpegDDP->hdlFFmpeg, avctx, codec, NULL);
    if (ret < 0)
    {
        PrintErrMsg(hdlFFmpegDDP->hdlFFmpeg, ret);
        return AVERROR(ret);
    }

    AVERROR(ret);
    return 0;
}

static void TestAudio()
{
    if (FFmpegDDPInstance.hdlFFmpeg == NULL)
    {
        return;
    }

    InitFFmpeg();

    AVFormatContext *ic = DFFmpeg_avformat_alloc_context(FFmpegDDPInstance.hdlFFmpeg);
    ic->interrupt_callback.callback = decode_interrupt_cb;
    ic->interrupt_callback.opaque = NULL;
    FFmpegDDPInstance.ic = ic;

    int ret = DFFmpeg_avformat_open_input(FFmpegDDPInstance.hdlFFmpeg, &ic, g_FILE_PATH, NULL, NULL);
    if (ret < 0)
    {
        PrintErrMsg(FFmpegDDPInstance.hdlFFmpeg, ret);
        return;
    }

    DFFmpeg_av_format_inject_global_side_data(FFmpegDDPInstance.hdlFFmpeg, ic);

    int orig_nb_streams = ic->nb_streams;
    int st_index[AVMEDIA_TYPE_NB];
    char* wanted_stream_spec[AVMEDIA_TYPE_NB] = {0};
    memset(st_index, -1, sizeof(st_index));

    ret = DFFmpeg_av_find_best_stream(FFmpegDDPInstance.hdlFFmpeg, ic, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if (ret < 0)
    {
        PrintErrMsg(FFmpegDDPInstance.hdlFFmpeg, ret);
        // return;
    }
    // st_index[AVMEDIA_TYPE_AUDIO] = ret;
    st_index[AVMEDIA_TYPE_AUDIO] = 0;

    if (st_index[AVMEDIA_TYPE_AUDIO] >= 0)
    {
        open_decoder(&FFmpegDDPInstance, st_index[AVMEDIA_TYPE_AUDIO]);
    }

    FFmpegDDPInstance.frame = DFFmpeg_av_frame_alloc(FFmpegDDPInstance.hdlFFmpeg);
    AVPacket pkt1, *pkt = &pkt1;
    AVRational tb = FFmpegDDPInstance.ic->streams[0]->time_base;
    do
    {
        ret = DFFmpeg_av_read_frame(FFmpegDDPInstance.hdlFFmpeg, ic, pkt);
        if (ret < 0)
        {
            PrintErrMsg(FFmpegDDPInstance.hdlFFmpeg, ret);
            break;
        }
        DLog(DLOG_D, TAG, "read bytes:%d, %lld", pkt->size, 1000 * pkt->pts * tb.num / tb.den);

        // send to decode
        if (Send2Decode(&FFmpegDDPInstance, pkt) < 0)
        {
            break;
        }

        // get decode output
        if (GetDecodeOutput(&FFmpegDDPInstance, FFmpegDDPInstance.frame) < 0)
        {
            break;
        }
    } while (1);

    DFFmpeg_av_frame_free(FFmpegDDPInstance.hdlFFmpeg, &FFmpegDDPInstance.frame);
}

static void TestInit()
{
    // clear log
    DLogFlush();
    DFileFlush(gPCMOutputPath);

    // load FFmpeg libraries
    FFmpegDDPInstance.hdlFFmpeg = DFFmpegInit();
}

static void TestRelease()
{
    // deload FFmpeg libraries
    DFFmpegRelease(&FFmpegDDPInstance.hdlFFmpeg);
}

void TestDecoder()
{
    DLog(DLOG_D, TAG, "Test begin");

    TestInit();

    TestAudio();

    TestRelease();

    DLog(DLOG_D, TAG, "Test end");
}
