#include <stdlib.h>
#include "DLoad.h"
#include "DLog.h"
#include "DFFmpeg.h"

#define TAG     "DFFmpeg"

typedef struct
{
    // dll handler
    void *hdlAVCodec;
    void *hdlAVFormat;
    void *hdlAVUtil;
    void *hdlSWResamble;

    // process handler
    av_register_all_proc av_register_all;
    av_format_inject_global_side_data_proc av_format_inject_global_side_data;
    avformat_alloc_context_proc avformat_alloc_context;
    avformat_open_input_proc avformat_open_input;
    av_find_best_stream_proc av_find_best_stream;
    av_read_frame_proc av_read_frame;

    av_codec_set_pkt_timebase_proc av_codec_set_pkt_timebase;
    av_codec_get_max_lowres_proc av_codec_get_max_lowres;
    avcodec_alloc_context3_proc avcodec_alloc_context3;
    avcodec_parameters_to_context_proc avcodec_parameters_to_context;
    avcodec_open2_proc avcodec_open2;
    avcodec_find_decoder_proc avcodec_find_decoder;
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

    hdlFFmpeg->av_register_all = (av_register_all_proc)DLoadGetSymbol(hdlFFmpeg->hdlAVFormat, "av_register_all");
    hdlFFmpeg->av_format_inject_global_side_data = (av_format_inject_global_side_data_proc)DLoadGetSymbol(hdlFFmpeg->hdlAVFormat, "av_format_inject_global_side_data");
    hdlFFmpeg->avformat_alloc_context = (avformat_alloc_context_proc)DLoadGetSymbol(hdlFFmpeg->hdlAVFormat, "avformat_alloc_context");
    hdlFFmpeg->avformat_open_input = (avformat_open_input_proc)DLoadGetSymbol(hdlFFmpeg->hdlAVFormat, "avformat_open_input");
    hdlFFmpeg->av_find_best_stream = (av_find_best_stream_proc)DLoadGetSymbol(hdlFFmpeg->hdlAVFormat, "av_find_best_stream");
    hdlFFmpeg->av_read_frame = (av_read_frame_proc)DLoadGetSymbol(hdlFFmpeg->hdlAVFormat, "av_read_frame");

    hdlFFmpeg->av_codec_set_pkt_timebase = (av_codec_set_pkt_timebase_proc)DLoadGetSymbol(hdlFFmpeg->hdlAVCodec, "av_codec_set_pkt_timebase");
    hdlFFmpeg->av_codec_get_max_lowres = (av_codec_get_max_lowres_proc)DLoadGetSymbol(hdlFFmpeg->hdlAVCodec, "av_codec_get_max_lowres");
    hdlFFmpeg->avcodec_alloc_context3 = (avcodec_alloc_context3_proc)DLoadGetSymbol(hdlFFmpeg->hdlAVCodec, "avcodec_alloc_context3");
    hdlFFmpeg->avcodec_parameters_to_context = (avcodec_parameters_to_context_proc)DLoadGetSymbol(hdlFFmpeg->hdlAVCodec, "avcodec_parameters_to_context");
    hdlFFmpeg->avcodec_open2 = (avcodec_open2_proc)DLoadGetSymbol(hdlFFmpeg->hdlAVCodec, "avcodec_open2");
    hdlFFmpeg->avcodec_find_decoder = (avcodec_find_decoder_proc)DLoadGetSymbol(hdlFFmpeg->hdlAVCodec, "avcodec_find_decoder");
    hdlFFmpeg->avcodec_send_packet = (avcodec_send_packet_proc)DLoadGetSymbol(hdlFFmpeg->hdlAVCodec, "avcodec_send_packet");
    hdlFFmpeg->avcodec_receive_frame = (avcodec_receive_frame_proc)DLoadGetSymbol(hdlFFmpeg->hdlAVCodec, "avcodec_receive_frame");

    hdlFFmpeg->av_strerror = (av_strerror_proc)DLoadGetSymbol(hdlFFmpeg->hdlAVUtil, "av_strerror");
    hdlFFmpeg->av_freep = (av_freep_proc)DLoadGetSymbol(hdlFFmpeg->hdlAVUtil, "av_freep");
    hdlFFmpeg->av_samples_alloc = (av_samples_alloc_proc)DLoadGetSymbol(hdlFFmpeg->hdlAVUtil, "av_samples_alloc");
    hdlFFmpeg->av_frame_alloc = (av_frame_alloc_proc)DLoadGetSymbol(hdlFFmpeg->hdlAVUtil, "av_frame_alloc");
    hdlFFmpeg->av_frame_free = (av_frame_free_proc)DLoadGetSymbol(hdlFFmpeg->hdlAVUtil, "av_frame_free");
    hdlFFmpeg->av_opt_set_int = (av_opt_set_int_proc)DLoadGetSymbol(hdlFFmpeg->hdlAVUtil, "av_opt_set_int");
    hdlFFmpeg->av_opt_set_sample_fmt = (av_opt_set_sample_fmt_proc)DLoadGetSymbol(hdlFFmpeg->hdlAVUtil, "av_opt_set_sample_fmt");

    hdlFFmpeg->swr_alloc = (swr_alloc_proc)DLoadGetSymbol(hdlFFmpeg->hdlSWResamble, "swr_alloc");
    hdlFFmpeg->swr_init = (swr_init_proc)DLoadGetSymbol(hdlFFmpeg->hdlSWResamble, "swr_init");
    hdlFFmpeg->swr_free = (swr_free_proc)DLoadGetSymbol(hdlFFmpeg->hdlSWResamble, "swr_free");
    hdlFFmpeg->swr_convert = (swr_convert_proc)DLoadGetSymbol(hdlFFmpeg->hdlSWResamble, "swr_convert");

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
        return;
    }

    DFFmpeg *hdlFFmpeg = (DFFmpeg*)hdl;
    if (hdlFFmpeg->avformat_alloc_context != NULL)
    {
        (*hdlFFmpeg->avformat_alloc_context)();
    }
}

DEXPORT int DFFmpeg_avformat_open_input(void *hdl, AVFormatContext **ps, const char *url, AVInputFormat *fmt, AVDictionary **options)
{
    if (hdl == NULL)
    {
        return;
    }

    DFFmpeg *hdlFFmpeg = (DFFmpeg*)hdl;
    if (hdlFFmpeg->avformat_open_input != NULL)
    {
        (*hdlFFmpeg->avformat_open_input)(ps, url, fmt, options);
    }
}

DEXPORT int DFFmpeg_av_find_best_stream(void *hdl, AVFormatContext *ic, enum AVMediaType type, int wanted_stream_nb, int related_stream, AVCodec **decoder_ret, int flags)
{
    if (hdl == NULL)
    {
        return;
    }

    DFFmpeg *hdlFFmpeg = (DFFmpeg*)hdl;
    if (hdlFFmpeg->av_find_best_stream != NULL)
    {
        (*hdlFFmpeg->av_find_best_stream)(ic, type, wanted_stream_nb, related_stream, decoder_ret, flags);
    }
}

DEXPORT int DFFmpeg_av_read_frame(void *hdl, AVFormatContext *s, AVPacket *pkt)
{
    if (hdl == NULL)
    {
        return;
    }

    DFFmpeg *hdlFFmpeg = (DFFmpeg*)hdl;
    if (hdlFFmpeg->av_read_frame != NULL)
    {
        (*hdlFFmpeg->av_read_frame)(s, pkt);
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
        return;
    }

    DFFmpeg *hdlFFmpeg = (DFFmpeg*)hdl;
    if (hdlFFmpeg->av_codec_get_max_lowres != NULL)
    {
        (*hdlFFmpeg->av_codec_get_max_lowres)(codec);
    }
}

DEXPORT AVCodecContext *DFFmpeg_avcodec_alloc_context3(void *hdl, const AVCodec *codec)
{
    if (hdl == NULL)
    {
        return;
    }

    DFFmpeg *hdlFFmpeg = (DFFmpeg*)hdl;
    if (hdlFFmpeg->avcodec_alloc_context3 != NULL)
    {
        (*hdlFFmpeg->avcodec_alloc_context3)(codec);
    }
}

DEXPORT int DFFmpeg_avcodec_parameters_to_context(void *hdl, AVCodecContext *codec, const AVCodecParameters *par)
{
    if (hdl == NULL)
    {
        return;
    }

    DFFmpeg *hdlFFmpeg = (DFFmpeg*)hdl;
    if (hdlFFmpeg->avcodec_parameters_to_context != NULL)
    {
        (*hdlFFmpeg->avcodec_parameters_to_context)(codec, par);
    }
}

DEXPORT int DFFmpeg_avcodec_open2(void *hdl, AVCodecContext *avctx, const AVCodec *codec, AVDictionary **options)
{
    if (hdl == NULL)
    {
        return;
    }

    DFFmpeg *hdlFFmpeg = (DFFmpeg*)hdl;
    if (hdlFFmpeg->avcodec_open2 != NULL)
    {
        (*hdlFFmpeg->avcodec_open2)(avctx, codec, options);
    }
}

DEXPORT AVCodec *DFFmpeg_avcodec_find_decoder(void *hdl, enum AVCodecID id)
{
    if (hdl == NULL)
    {
        return;
    }

    DFFmpeg *hdlFFmpeg = (DFFmpeg*)hdl;
    if (hdlFFmpeg->avcodec_find_decoder != NULL)
    {
        (*hdlFFmpeg->avcodec_find_decoder)(id);
    }
}

DEXPORT int DFFmpeg_avcodec_send_packet(void *hdl, AVCodecContext *avctx, const AVPacket *avpkt)
{
    if (hdl == NULL)
    {
        return;
    }

    DFFmpeg *hdlFFmpeg = (DFFmpeg*)hdl;
    if (hdlFFmpeg->avcodec_send_packet != NULL)
    {
        (*hdlFFmpeg->avcodec_send_packet)(avctx, avpkt);
    }
}

DEXPORT int DFFmpeg_avcodec_receive_frame(void *hdl, AVCodecContext *avctx, AVFrame *frame)
{
    if (hdl == NULL)
    {
        return;
    }

    DFFmpeg *hdlFFmpeg = (DFFmpeg*)hdl;
    if (hdlFFmpeg->avcodec_receive_frame != NULL)
    {
        (*hdlFFmpeg->avcodec_receive_frame)(avctx, frame);
    }
}

DEXPORT int DFFmpeg_av_strerror(void *hdl, int errnum, char *errbuf, size_t errbuf_size)
{
    if (hdl == NULL)
    {
        return;
    }

    DFFmpeg *hdlFFmpeg = (DFFmpeg*)hdl;
    if (hdlFFmpeg->av_strerror != NULL)
    {
        (*hdlFFmpeg->av_strerror)(errnum, errbuf, errbuf_size);
    }
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
        return;
    }

    DFFmpeg *hdlFFmpeg = (DFFmpeg*)hdl;
    if (hdlFFmpeg->av_samples_alloc != NULL)
    {
        (*hdlFFmpeg->av_samples_alloc)(audio_data, linesize, nb_channels, nb_samples, sample_fmt, align);
    }
}

DEXPORT AVFrame *DFFmpeg_av_frame_alloc(void *hdl)
{
    if (hdl == NULL)
    {
        return;
    }

    DFFmpeg *hdlFFmpeg = (DFFmpeg*)hdl;
    if (hdlFFmpeg->av_frame_alloc != NULL)
    {
        (*hdlFFmpeg->av_frame_alloc)();
    }
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
        return;
    }

    DFFmpeg *hdlFFmpeg = (DFFmpeg*)hdl;
    if (hdlFFmpeg->av_opt_set_int != NULL)
    {
        (*hdlFFmpeg->av_opt_set_int)(obj, name, val, search_flags);
    }
}

DEXPORT int DFFmpeg_av_opt_set_sample_fmt(void *hdl, void *obj, const char *name, enum AVSampleFormat fmt, int search_flags)
{
    if (hdl == NULL)
    {
        return;
    }

    DFFmpeg *hdlFFmpeg = (DFFmpeg*)hdl;
    if (hdlFFmpeg->av_opt_set_sample_fmt != NULL)
    {
        (*hdlFFmpeg->av_opt_set_sample_fmt)(obj, name, fmt, search_flags);
    }
}

DEXPORT struct SwrContext *DFFmpeg_swr_alloc(void *hdl)
{
    if (hdl == NULL)
    {
        return;
    }

    DFFmpeg *hdlFFmpeg = (DFFmpeg*)hdl;
    if (hdlFFmpeg->swr_alloc != NULL)
    {
        (*hdlFFmpeg->swr_alloc)();
    }
}

DEXPORT int DFFmpeg_swr_init(void *hdl, struct SwrContext *s)
{
    if (hdl == NULL)
    {
        return;
    }

    DFFmpeg *hdlFFmpeg = (DFFmpeg*)hdl;
    if (hdlFFmpeg->swr_init != NULL)
    {
        (*hdlFFmpeg->swr_init)(s);
    }
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
        return;
    }

    DFFmpeg *hdlFFmpeg = (DFFmpeg*)hdl;
    if (hdlFFmpeg->swr_convert != NULL)
    {
        (*hdlFFmpeg->swr_convert)(s, out, out_count, in, in_count);
    }
}
