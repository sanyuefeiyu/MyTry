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
DEXPORT int DAOOpen(void *ao, AudioAttr *audioAttr);
DEXPORT int DAOWrite(void *ao, DPCM *pcm);
DEXPORT void DAOClose(void *ao);

#ifdef __cplusplus
}
#endif

#endif /* D_AUDIO_OUT_H */
