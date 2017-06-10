/*
 * @author Double
 * @since 2017/06/11
 */

#ifndef D_AUDIO_OUT_H
#define D_AUDIO_OUT_H

#include "DExport.h"
#include "DPCM.h"

#ifdef __cplusplus
extern "C" {
#endif

DEXPORT void* DAOInit();
DEXPORT void DAORelease(void **ao);
DEXPORT void DAOOpen(void *ao);
DEXPORT void DAWrite(void *ao, DPCM *pcm);
DEXPORT void DAOClose(void *ao);

#ifdef __cplusplus
}
#endif

#endif /* D_AUDIO_OUT_H */
