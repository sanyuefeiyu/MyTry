/*
 * @author Double
 * @since 2017/05/13
 */

#ifndef D_FFMPEG_H
#define D_FFMPEG_H

#include "DExport.h"

#ifdef __cplusplus
extern "C" {
#endif

DEXPORT void* DFFmpegInit();
DEXPORT void DFFmpegRelease(void **hdl);

#ifdef __cplusplus
}
#endif

#endif /* D_FFMPEG_H */
