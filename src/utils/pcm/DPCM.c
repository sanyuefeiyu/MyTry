#include <stdlib.h>
#include <string.h>
#include "DPCM.h"
#include "DLog.h"

#define TAG     "DPCM"

#define MAX_PCM_SIZE    (1*1024*1024)

DEXPORT void DPCMAdd(DPCM *pcm, unsigned char *buf, unsigned int size, AudioAttr *audioAttr)
{
    if (pcm == NULL || buf == NULL || size <= 0 || audioAttr == NULL)
    {
        return;
    }

    // init PCM buffer
    if (pcm->data == NULL)
    {
        pcm->data = malloc(size);
        if (pcm->data == NULL)
        {
            DLog(DLOG_W, TAG, "alloc PCM buffer [%u] failed", size);
            return;
        }
        pcm->capacity = size;
        pcm->size = 0;
    }

    // realloc PCM buffer
    if (size + pcm->size > pcm->capacity)
    {
        // reach at max PCM size, and do not malloc again
        if (pcm->capacity >= MAX_PCM_SIZE)
        {
            DLog(DLOG_W, TAG, "reach at max PCM size[%u], and do not malloc again", pcm->capacity);
            return;
        }

        char *tempBuff = malloc(size + pcm->size);
        if (tempBuff == NULL)
        {
            DLog(DLOG_W, TAG, "realloc PCM buffer [%u,%u] failed", size, pcm->size);
            return;
        }

        memcpy(tempBuff, pcm->data, pcm->size);
        free(pcm->data);
        pcm->data = tempBuff;
        pcm->capacity = size + pcm->size;
    }

    // copy buffer to PCM
    memcpy(pcm->data + pcm->size, buf, size);
    pcm->size += size;

    pcm->audioAttr.sampleRate = audioAttr->sampleRate;
    pcm->audioAttr.channels = audioAttr->channels;
    pcm->audioAttr.sampleBits = audioAttr->sampleBits;
}

DEXPORT void DPCMClean(DPCM *pcm)
{
    if (pcm == NULL)
    {
        return;
    }

    pcm->size = 0;
    pcm->audioAttr.sampleRate = 0;
    pcm->audioAttr.channels = 0;
    pcm->audioAttr.sampleBits = 0;
}

DEXPORT void DPCMReset(DPCM *pcm)
{
    if (pcm == NULL)
    {
        return;
    }

    // reset buffer
    if (pcm->data != NULL)
    {
        free(pcm->data);
        pcm->data = NULL;
    }
    pcm->capacity = 0;
    pcm->size = 0;
    pcm->audioAttr.sampleRate = 0;
    pcm->audioAttr.channels = 0;
    pcm->audioAttr.sampleBits = 0;
}
