/*
 * @author Double
 * @since 2017/05/13
 */

#ifndef D_FFMPEG_H
#define D_FFMPEG_H

#include "DExport.h"

#define inline __inline
#include "libavformat\avformat.h"
#include "libswresample\swresample.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*av_register_all_proc)(void);
typedef void (*av_format_inject_global_side_data_proc)(AVFormatContext *s);
typedef AVFormatContext *(*avformat_alloc_context_proc)(void);
typedef int (*avformat_open_input_proc)(AVFormatContext **ps, const char *url, AVInputFormat *fmt, AVDictionary **options);
typedef int (*av_find_best_stream_proc)(AVFormatContext *ic, enum AVMediaType type, int wanted_stream_nb, int related_stream, AVCodec **decoder_ret, int flags);
typedef int (*av_read_frame_proc)(AVFormatContext *s, AVPacket *pkt);

typedef void (*av_codec_set_pkt_timebase_proc)(AVCodecContext *avctx, AVRational val);
typedef int (*av_codec_get_max_lowres_proc)(const AVCodec *codec);
typedef AVCodecContext *(*avcodec_alloc_context3_proc)(const AVCodec *codec);
typedef int (*avcodec_parameters_to_context_proc)(AVCodecContext *codec, const AVCodecParameters *par);
typedef int (*avcodec_open2_proc)(AVCodecContext *avctx, const AVCodec *codec, AVDictionary **options);
typedef int (*avcodec_close_proc)(AVCodecContext *avctx);
typedef AVCodec *(*avcodec_find_decoder_proc)(enum AVCodecID id);
typedef AVCodec *(*avcodec_find_decoder_by_name_proc)(const char *name);
typedef int (*avcodec_send_packet_proc)(AVCodecContext *avctx, const AVPacket *avpkt);
typedef int (*avcodec_receive_frame_proc)(AVCodecContext *avctx, AVFrame *frame);

typedef int (*av_strerror_proc)(int errnum, char *errbuf, size_t errbuf_size);
typedef void (*av_freep_proc)(void *arg);
typedef int(*av_samples_alloc_proc)(uint8_t **audio_data, int *linesize, int nb_channels, int nb_samples, enum AVSampleFormat sample_fmt, int align);
typedef AVFrame *(*av_frame_alloc_proc)(void);
typedef void (*av_frame_free_proc)(AVFrame **frame);
typedef int (*av_opt_set_int_proc)(void *obj, const char *name, int64_t     val, int search_flags);
typedef int (*av_opt_set_sample_fmt_proc)(void *obj, const char *name, enum AVSampleFormat fmt, int search_flags);

typedef struct SwrContext *(*swr_alloc_proc)(void);
typedef int (*swr_init_proc)(struct SwrContext *s);
typedef av_cold void (*swr_free_proc)(SwrContext **ss);
typedef int (*swr_convert_proc)(struct SwrContext *s, uint8_t **out, int out_count, const uint8_t **in, int in_count);

DEXPORT void* DFFmpegInit();
DEXPORT void DFFmpegRelease(void **hdl);

DEXPORT void DFFmpeg_av_register_all(void *hdl);
DEXPORT void DFFmpeg_av_format_inject_global_side_data(void *hdl, AVFormatContext *s);
DEXPORT AVFormatContext *DFFmpeg_avformat_alloc_context(void *hdl);
DEXPORT int DFFmpeg_avformat_open_input(void *hdl, AVFormatContext **ps, const char *url, AVInputFormat *fmt, AVDictionary **options);
DEXPORT int DFFmpeg_av_find_best_stream(void *hdl, AVFormatContext *ic, enum AVMediaType type, int wanted_stream_nb, int related_stream, AVCodec **decoder_ret, int flags);
DEXPORT int DFFmpeg_av_read_frame(void *hdl, AVFormatContext *s, AVPacket *pkt);

DEXPORT void DFFmpeg_av_codec_set_pkt_timebase(void *hdl, AVCodecContext *avctx, AVRational val);
DEXPORT int DFFmpeg_av_codec_get_max_lowres(void *hdl, const AVCodec *codec);
DEXPORT AVCodecContext *DFFmpeg_avcodec_alloc_context3(void *hdl, const AVCodec *codec);
DEXPORT int DFFmpeg_avcodec_parameters_to_context(void *hdl, AVCodecContext *codec, const AVCodecParameters *par);
DEXPORT int DFFmpeg_avcodec_open2(void *hdl, AVCodecContext *avctx, const AVCodec *codec, AVDictionary **options);
DEXPORT int DFFmpeg_avcodec_close(void *hdl, AVCodecContext *avctx);
DEXPORT AVCodec *DFFmpeg_avcodec_find_decoder(void *hdl, enum AVCodecID id);
DEXPORT AVCodec *DFFmpeg_avcodec_find_decoder_by_name(void *hdl, const char *name);
DEXPORT int DFFmpeg_avcodec_send_packet(void *hdl, AVCodecContext *avctx, const AVPacket *avpkt);
DEXPORT int DFFmpeg_avcodec_receive_frame(void *hdl, AVCodecContext *avctx, AVFrame *frame);

DEXPORT int DFFmpeg_av_strerror(void *hdl, int errnum, char *errbuf, size_t errbuf_size);
DEXPORT void DFFmpeg_av_freep(void *hdl, void *arg);
DEXPORT int DFFmpeg_av_samples_alloc(void *hdl, uint8_t **audio_data, int *linesize, int nb_channels, int nb_samples, enum AVSampleFormat sample_fmt, int align);
DEXPORT AVFrame *DFFmpeg_av_frame_alloc(void *hdl);
DEXPORT void DFFmpeg_av_frame_free(void *hdl, AVFrame **frame);
DEXPORT int DFFmpeg_av_opt_set_int(void *hdl, void *obj, const char *name, int64_t     val, int search_flags);
DEXPORT int DFFmpeg_av_opt_set_sample_fmt(void *hdl, void *obj, const char *name, enum AVSampleFormat fmt, int search_flags);

DEXPORT struct SwrContext *DFFmpeg_swr_alloc(void *hdl);
DEXPORT int DFFmpeg_swr_init(void *hdl, struct SwrContext *s);
DEXPORT av_cold void DFFmpeg_swr_free(void *hdl, SwrContext **ss);
DEXPORT int DFFmpeg_swr_convert(void *hdl, struct SwrContext *s, uint8_t **out, int out_count, const uint8_t **in, int in_count);

#ifdef __cplusplus
}
#endif

#endif /* D_FFMPEG_H */
