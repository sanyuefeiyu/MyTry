#include <stdlib.h>
#include "DLoad.h"
#include "DLog.h"
#include "DFFmpeg.h"

#define TAG     "DFFmpeg"

typedef struct
{
    // dll handler
    void *ffmpeg;

    // process handler
    av_register_all_proc av_register_all;
    av_format_inject_global_side_data_proc av_format_inject_global_side_data;
    avformat_alloc_context_proc avformat_alloc_context;
    avformat_open_input_proc avformat_open_input;
    av_find_best_stream_proc av_find_best_stream;
    av_read_frame_proc av_read_frame;

    avcodec_register_all_proc avcodec_register_all;
    av_codec_set_pkt_timebase_proc av_codec_set_pkt_timebase;
    av_codec_get_max_lowres_proc av_codec_get_max_lowres;
    avcodec_alloc_context3_proc avcodec_alloc_context3;
    avcodec_parameters_to_context_proc avcodec_parameters_to_context;
    avcodec_open2_proc avcodec_open2;
    avcodec_close_proc avcodec_close;
    avcodec_find_decoder_proc avcodec_find_decoder;
    avcodec_find_decoder_by_name_proc avcodec_find_decoder_by_name;
    avcodec_send_packet_proc avcodec_send_packet;
    avcodec_receive_frame_proc avcodec_receive_frame;

    av_strerror_proc av_strerror;
    av_freep_proc av_freep;
    av_samples_alloc_proc av_samples_alloc;
    av_frame_alloc_proc av_frame_alloc;
    av_frame_free_proc av_frame_free;
    av_opt_set_int_proc av_opt_set_int;
    av_opt_set_sample_fmt_proc av_opt_set_sample_fmt;

    swr_alloc_proc swr_alloc;
    swr_init_proc swr_init;
    swr_free_proc swr_free;
    swr_convert_proc swr_convert;
} DFFmpeg;

DEXPORT void* DFFmpegInit()
{
    DFFmpeg *hdlFFmpeg = calloc(1, sizeof(DFFmpeg));
    if (hdlFFmpeg == NULL)
    {
        return NULL;
    }

    hdlFFmpeg->ffmpeg = DLoadOpen("libffmpeg_ddp.so");

    if (hdlFFmpeg->ffmpeg == NULL)
    {
        DFFmpegRelease((void**)&hdlFFmpeg);
        return NULL;
    }

    hdlFFmpeg->avcodec_register_all = (avcodec_register_all_proc)DLoadGetSymbol(hdlFFmpeg->ffmpeg, "avcodec_register_all");
    hdlFFmpeg->av_codec_set_pkt_timebase = (av_codec_set_pkt_timebase_proc)DLoadGetSymbol(hdlFFmpeg->ffmpeg, "av_codec_set_pkt_timebase");
    hdlFFmpeg->av_codec_get_max_lowres = (av_codec_get_max_lowres_proc)DLoadGetSymbol(hdlFFmpeg->ffmpeg, "av_codec_get_max_lowres");
    hdlFFmpeg->avcodec_alloc_context3 = (avcodec_alloc_context3_proc)DLoadGetSymbol(hdlFFmpeg->ffmpeg, "avcodec_alloc_context3");
    hdlFFmpeg->avcodec_parameters_to_context = (avcodec_parameters_to_context_proc)DLoadGetSymbol(hdlFFmpeg->ffmpeg, "avcodec_parameters_to_context");
    hdlFFmpeg->avcodec_open2 = (avcodec_open2_proc)DLoadGetSymbol(hdlFFmpeg->ffmpeg, "avcodec_open2");
    hdlFFmpeg->avcodec_close = (avcodec_close_proc)DLoadGetSymbol(hdlFFmpeg->ffmpeg, "avcodec_close");
    hdlFFmpeg->avcodec_find_decoder = (avcodec_find_decoder_proc)DLoadGetSymbol(hdlFFmpeg->ffmpeg, "avcodec_find_decoder");
    hdlFFmpeg->avcodec_find_decoder_by_name = (avcodec_find_decoder_by_name_proc)DLoadGetSymbol(hdlFFmpeg->ffmpeg, "avcodec_find_decoder_by_name");
    hdlFFmpeg->avcodec_send_packet = (avcodec_send_packet_proc)DLoadGetSymbol(hdlFFmpeg->ffmpeg, "avcodec_send_packet");
    hdlFFmpeg->avcodec_receive_frame = (avcodec_receive_frame_proc)DLoadGetSymbol(hdlFFmpeg->ffmpeg, "avcodec_receive_frame");

    hdlFFmpeg->av_strerror = (av_strerror_proc)DLoadGetSymbol(hdlFFmpeg->ffmpeg, "av_strerror");
    hdlFFmpeg->av_freep = (av_freep_proc)DLoadGetSymbol(hdlFFmpeg->ffmpeg, "av_freep");
    hdlFFmpeg->av_samples_alloc = (av_samples_alloc_proc)DLoadGetSymbol(hdlFFmpeg->ffmpeg, "av_samples_alloc");
    hdlFFmpeg->av_frame_alloc = (av_frame_alloc_proc)DLoadGetSymbol(hdlFFmpeg->ffmpeg, "av_frame_alloc");
    hdlFFmpeg->av_frame_free = (av_frame_free_proc)DLoadGetSymbol(hdlFFmpeg->ffmpeg, "av_frame_free");
    hdlFFmpeg->av_opt_set_int = (av_opt_set_int_proc)DLoadGetSymbol(hdlFFmpeg->ffmpeg, "av_opt_set_int");
    hdlFFmpeg->av_opt_set_sample_fmt = (av_opt_set_sample_fmt_proc)DLoadGetSymbol(hdlFFmpeg->ffmpeg, "av_opt_set_sample_fmt");

    hdlFFmpeg->swr_alloc = (swr_alloc_proc)DLoadGetSymbol(hdlFFmpeg->ffmpeg, "swr_alloc");
    hdlFFmpeg->swr_init = (swr_init_proc)DLoadGetSymbol(hdlFFmpeg->ffmpeg, "swr_init");
    hdlFFmpeg->swr_free = (swr_free_proc)DLoadGetSymbol(hdlFFmpeg->ffmpeg, "swr_free");
    hdlFFmpeg->swr_convert = (swr_convert_proc)DLoadGetSymbol(hdlFFmpeg->ffmpeg, "swr_convert");

    return hdlFFmpeg;
}

DEXPORT void DFFmpegRelease(void **hdl)
{
    if (hdl == NULL || *hdl == NULL)
    {
        return;
    }

    DFFmpeg *hdlFFmpeg = *hdl;

    if (hdlFFmpeg->ffmpeg != NULL)
    {
        DLoadClose(hdlFFmpeg->ffmpeg);
    }

    free(hdlFFmpeg);
    *hdl = NULL;
}

DEXPORT void DFFmpeg_av_register_all(void *hdl)
{
    if (hdl == NULL)
    {
        return;
    }

    DFFmpeg *hdlFFmpeg = (DFFmpeg*)hdl;
    if (hdlFFmpeg->av_register_all != NULL)
    {
        (*hdlFFmpeg->av_register_all)();
    }
}

DEXPORT void DFFmpeg_av_format_inject_global_side_data(void *hdl, AVFormatContext *s)
{
    if (hdl == NULL)
    {
        return;
    }

    DFFmpeg *hdlFFmpeg = (DFFmpeg*)hdl;
    if (hdlFFmpeg->av_format_inject_global_side_data != NULL)
    {
        (*hdlFFmpeg->av_format_inject_global_side_data)(s);
    }
}

DEXPORT AVFormatContext *DFFmpeg_avformat_alloc_context(void *hdl)
{
    if (hdl == NULL)
    {
        return NULL;
    }

    DFFmpeg *hdlFFmpeg = (DFFmpeg*)hdl;
    if (hdlFFmpeg->avformat_alloc_context != NULL)
    {
        return (*hdlFFmpeg->avformat_alloc_context)();
    }

    return NULL;
}

DEXPORT int DFFmpeg_avformat_open_input(void *hdl, AVFormatContext **ps, const char *url, AVInputFormat *fmt, AVDictionary **options)
{
    if (hdl == NULL)
    {
        return AVERROR_INVALIDDATA;
    }

    DFFmpeg *hdlFFmpeg = (DFFmpeg*)hdl;
    if (hdlFFmpeg->avformat_open_input != NULL)
    {
        return (*hdlFFmpeg->avformat_open_input)(ps, url, fmt, options);
    }

    return AVERROR_INVALIDDATA;
}

DEXPORT int DFFmpeg_av_find_best_stream(void *hdl, AVFormatContext *ic, enum AVMediaType type, int wanted_stream_nb, int related_stream, AVCodec **decoder_ret, int flags)
{
    if (hdl == NULL)
    {
        return AVERROR_INVALIDDATA;
    }

    DFFmpeg *hdlFFmpeg = (DFFmpeg*)hdl;
    if (hdlFFmpeg->av_find_best_stream != NULL)
    {
        return (*hdlFFmpeg->av_find_best_stream)(ic, type, wanted_stream_nb, related_stream, decoder_ret, flags);
    }

    return AVERROR_INVALIDDATA;
}

DEXPORT int DFFmpeg_av_read_frame(void *hdl, AVFormatContext *s, AVPacket *pkt)
{
    if (hdl == NULL)
    {
        return AVERROR_INVALIDDATA;
    }

    DFFmpeg *hdlFFmpeg = (DFFmpeg*)hdl;
    if (hdlFFmpeg->av_read_frame != NULL)
    {
        return (*hdlFFmpeg->av_read_frame)(s, pkt);
    }

    return AVERROR_INVALIDDATA;
}

DEXPORT void DFFmpeg_avcodec_register_all(void *hdl)
{
    if (hdl == NULL)
    {
        return;
    }

    DFFmpeg *hdlFFmpeg = (DFFmpeg*)hdl;
    if (hdlFFmpeg->avcodec_register_all != NULL)
    {
        (*hdlFFmpeg->avcodec_register_all)();
    }
}

DEXPORT void DFFmpeg_av_codec_set_pkt_timebase(void *hdl, AVCodecContext *avctx, AVRational val)
{
    if (hdl == NULL)
    {
        return;
    }

    DFFmpeg *hdlFFmpeg = (DFFmpeg*)hdl;
    if (hdlFFmpeg->av_codec_set_pkt_timebase != NULL)
    {
        (*hdlFFmpeg->av_codec_set_pkt_timebase)(avctx, val);
    }
}

DEXPORT int DFFmpeg_av_codec_get_max_lowres(void *hdl, const AVCodec *codec)
{
    if (hdl == NULL)
    {
        return AVERROR_INVALIDDATA;
    }

    DFFmpeg *hdlFFmpeg = (DFFmpeg*)hdl;
    if (hdlFFmpeg->av_codec_get_max_lowres != NULL)
    {
        return (*hdlFFmpeg->av_codec_get_max_lowres)(codec);
    }

    return AVERROR_INVALIDDATA;
}

DEXPORT AVCodecContext *DFFmpeg_avcodec_alloc_context3(void *hdl, const AVCodec *codec)
{
    if (hdl == NULL)
    {
        return NULL;
    }

    DFFmpeg *hdlFFmpeg = (DFFmpeg*)hdl;
    if (hdlFFmpeg->avcodec_alloc_context3 != NULL)
    {
        return (*hdlFFmpeg->avcodec_alloc_context3)(codec);
    }

    return NULL;
}

DEXPORT int DFFmpeg_avcodec_parameters_to_context(void *hdl, AVCodecContext *codec, const AVCodecParameters *par)
{
    if (hdl == NULL)
    {
        return AVERROR_INVALIDDATA;
    }

    DFFmpeg *hdlFFmpeg = (DFFmpeg*)hdl;
    if (hdlFFmpeg->avcodec_parameters_to_context != NULL)
    {
        return (*hdlFFmpeg->avcodec_parameters_to_context)(codec, par);
    }

    return AVERROR_INVALIDDATA;
}

DEXPORT int DFFmpeg_avcodec_open2(void *hdl, AVCodecContext *avctx, const AVCodec *codec, AVDictionary **options)
{
    if (hdl == NULL)
    {
        return AVERROR_INVALIDDATA;
    }

    DFFmpeg *hdlFFmpeg = (DFFmpeg*)hdl;
    if (hdlFFmpeg->avcodec_open2 != NULL)
    {
        return (*hdlFFmpeg->avcodec_open2)(avctx, codec, options);
    }

    return AVERROR_INVALIDDATA;
}

DEXPORT int DFFmpeg_avcodec_close(void *hdl, AVCodecContext *avctx)
{
    if (hdl == NULL)
    {
        return AVERROR_INVALIDDATA;
    }

    DFFmpeg *hdlFFmpeg = (DFFmpeg*)hdl;
    if (hdlFFmpeg->avcodec_close != NULL)
    {
        return (*hdlFFmpeg->avcodec_close)(avctx);
    }

    return AVERROR_INVALIDDATA;
}

DEXPORT AVCodec *DFFmpeg_avcodec_find_decoder(void *hdl, enum AVCodecID id)
{
    if (hdl == NULL)
    {
        return NULL;
    }

    DFFmpeg *hdlFFmpeg = (DFFmpeg*)hdl;
    if (hdlFFmpeg->avcodec_find_decoder != NULL)
    {
        return (*hdlFFmpeg->avcodec_find_decoder)(id);
    }

    return NULL;
}

DEXPORT AVCodec *DFFmpeg_avcodec_find_decoder_by_name(void *hdl, const char *name)
{
    if (hdl == NULL)
    {
        return NULL;
    }

    DFFmpeg *hdlFFmpeg = (DFFmpeg*)hdl;
    if (hdlFFmpeg->avcodec_find_decoder_by_name != NULL)
    {
        return (*hdlFFmpeg->avcodec_find_decoder_by_name)(name);
    }

    return NULL;
}

DEXPORT int DFFmpeg_avcodec_send_packet(void *hdl, AVCodecContext *avctx, const AVPacket *avpkt)
{
    if (hdl == NULL)
    {
        return AVERROR_INVALIDDATA;
    }

    DFFmpeg *hdlFFmpeg = (DFFmpeg*)hdl;
    if (hdlFFmpeg->avcodec_send_packet != NULL)
    {
        return (*hdlFFmpeg->avcodec_send_packet)(avctx, avpkt);
    }

    return AVERROR_INVALIDDATA;
}

DEXPORT int DFFmpeg_avcodec_receive_frame(void *hdl, AVCodecContext *avctx, AVFrame *frame)
{
    if (hdl == NULL)
    {
        return AVERROR_INVALIDDATA;
    }

    DFFmpeg *hdlFFmpeg = (DFFmpeg*)hdl;
    if (hdlFFmpeg->avcodec_receive_frame != NULL)
    {
        return (*hdlFFmpeg->avcodec_receive_frame)(avctx, frame);
    }

    return AVERROR_INVALIDDATA;
}

DEXPORT int DFFmpeg_av_strerror(void *hdl, int errnum, char *errbuf, size_t errbuf_size)
{
    if (hdl == NULL)
    {
        return AVERROR_INVALIDDATA;
    }

    DFFmpeg *hdlFFmpeg = (DFFmpeg*)hdl;
    if (hdlFFmpeg->av_strerror != NULL)
    {
        return (*hdlFFmpeg->av_strerror)(errnum, errbuf, errbuf_size);
    }

    return AVERROR_INVALIDDATA;
}

DEXPORT void DFFmpeg_av_freep(void *hdl, void *arg)
{
    if (hdl == NULL)
    {
        return;
    }

    DFFmpeg *hdlFFmpeg = (DFFmpeg*)hdl;
    if (hdlFFmpeg->av_freep != NULL)
    {
        (*hdlFFmpeg->av_freep)(arg);
    }
}

DEXPORT int DFFmpeg_av_samples_alloc(void *hdl, uint8_t **audio_data, int *linesize, int nb_channels, int nb_samples, enum AVSampleFormat sample_fmt, int align)
{
    if (hdl == NULL)
    {
        return AVERROR_INVALIDDATA;
    }

    DFFmpeg *hdlFFmpeg = (DFFmpeg*)hdl;
    if (hdlFFmpeg->av_samples_alloc != NULL)
    {
        return (*hdlFFmpeg->av_samples_alloc)(audio_data, linesize, nb_channels, nb_samples, sample_fmt, align);
    }

    return AVERROR_INVALIDDATA;
}

DEXPORT AVFrame *DFFmpeg_av_frame_alloc(void *hdl)
{
    if (hdl == NULL)
    {
        return NULL;
    }

    DFFmpeg *hdlFFmpeg = (DFFmpeg*)hdl;
    if (hdlFFmpeg->av_frame_alloc != NULL)
    {
        return (*hdlFFmpeg->av_frame_alloc)();
    }

    return NULL;
}

DEXPORT void DFFmpeg_av_frame_free(void *hdl, AVFrame **frame)
{
    if (hdl == NULL)
    {
        return;
    }

    DFFmpeg *hdlFFmpeg = (DFFmpeg*)hdl;
    if (hdlFFmpeg->av_frame_free != NULL)
    {
        (*hdlFFmpeg->av_frame_free)(frame);
    }
}

DEXPORT int DFFmpeg_av_opt_set_int(void *hdl, void *obj, const char *name, int64_t val, int search_flags)
{
    if (hdl == NULL)
    {
        return AVERROR_INVALIDDATA;
    }

    DFFmpeg *hdlFFmpeg = (DFFmpeg*)hdl;
    if (hdlFFmpeg->av_opt_set_int != NULL)
    {
        return (*hdlFFmpeg->av_opt_set_int)(obj, name, val, search_flags);
    }

    return AVERROR_INVALIDDATA;
}

DEXPORT int DFFmpeg_av_opt_set_sample_fmt(void *hdl, void *obj, const char *name, enum AVSampleFormat fmt, int search_flags)
{
    if (hdl == NULL)
    {
        return AVERROR_INVALIDDATA;
    }

    DFFmpeg *hdlFFmpeg = (DFFmpeg*)hdl;
    if (hdlFFmpeg->av_opt_set_sample_fmt != NULL)
    {
        return (*hdlFFmpeg->av_opt_set_sample_fmt)(obj, name, fmt, search_flags);
    }

    return AVERROR_INVALIDDATA;
}

DEXPORT struct SwrContext *DFFmpeg_swr_alloc(void *hdl)
{
    if (hdl == NULL)
    {
        return NULL;
    }

    DFFmpeg *hdlFFmpeg = (DFFmpeg*)hdl;
    if (hdlFFmpeg->swr_alloc != NULL)
    {
        return (*hdlFFmpeg->swr_alloc)();
    }

    return NULL;
}

DEXPORT int DFFmpeg_swr_init(void *hdl, struct SwrContext *s)
{
    if (hdl == NULL)
    {
        return AVERROR_INVALIDDATA;
    }

    DFFmpeg *hdlFFmpeg = (DFFmpeg*)hdl;
    if (hdlFFmpeg->swr_init != NULL)
    {
        return (*hdlFFmpeg->swr_init)(s);
    }

    return AVERROR_INVALIDDATA;
}

DEXPORT av_cold void DFFmpeg_swr_free(void *hdl, SwrContext **ss)
{
    if (hdl == NULL)
    {
        return;
    }

    DFFmpeg *hdlFFmpeg = (DFFmpeg*)hdl;
    if (hdlFFmpeg->swr_free != NULL)
    {
        (*hdlFFmpeg->swr_free)(ss);
    }
}

DEXPORT int DFFmpeg_swr_convert(void *hdl, struct SwrContext *s, uint8_t **out, int out_count, const uint8_t **in, int in_count)
{
    if (hdl == NULL)
    {
        return AVERROR_INVALIDDATA;
    }

    DFFmpeg *hdlFFmpeg = (DFFmpeg*)hdl;
    if (hdlFFmpeg->swr_convert != NULL)
    {
        return (*hdlFFmpeg->swr_convert)(s, out, out_count, in, in_count);
    }

    return AVERROR_INVALIDDATA;
}
