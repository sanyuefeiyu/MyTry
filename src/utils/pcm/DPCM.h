/*
 * @author Double
 * @since 2017/05/21
 */

#ifndef D_PCM_H
#define D_PCM_H

#include "DExport.h"

typedef struct DPCM
{
    char *data;
    unsigned int capacity;
    unsigned int size;
} DPCM;

#ifdef __cplusplus
extern "C" {
#endif

DEXPORT void DPCMAdd(DPCM *pcm, char *buff, unsigned int size);
DEXPORT void DPCMReset(DPCM *pcm);
DEXPORT void DPCMClean(DPCM *pcm);

#ifdef __cplusplus
}
#endif

#endif /* D_PCM_H */
