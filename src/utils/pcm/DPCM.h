/*
 * @author Double
 * @since 2017/05/21
 */

#ifndef D_PCM_H
#define D_PCM_H

#include "DExport.h"

typedef struct
{
    unsigned int sampleRate;
    unsigned int channels;
    unsigned int sampleBits;
} AudioAttr;

typedef struct
{
    unsigned char *data;
    unsigned int capacity;
    unsigned int size;

    AudioAttr audioAttr;
} DPCM;

#ifdef __cplusplus
extern "C" {
#endif

DEXPORT void DPCMAdd(DPCM *pcm, unsigned char *buf, unsigned int size, AudioAttr *audioAttr);
DEXPORT void DPCMClean(DPCM *pcm);
DEXPORT void DPCMReset(DPCM *pcm);

#ifdef __cplusplus
}
#endif

#endif /* D_PCM_H */
