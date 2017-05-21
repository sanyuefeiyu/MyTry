#include <stdlib.h>
#include <string.h>
#include "DPCM.h"
#include "DLog.h"

#define TAG     "DPCM"

#define MAX_PCM_SIZE    (10*1024*1024)
DEXPORT void DPCMAdd(DPCM *pcm, char *buff, unsigned int size)
{
    if (pcm == NULL || buff == NULL || size <=0)
    {
        return;
    }

    // init PCM buffer
    if (pcm->data == NULL)
    {
        pcm->data = malloc(size);
        if (pcm->data == NULL)
        {
            return;
        }
        pcm->capacity = size;
        pcm->size = 0;
    }

    // realloc PCM buffer
    if (size + pcm->size > pcm->capacity)
    {
        // reach at max presize, and do not malloc again
        if (pcm->capacity >= MAX_PCM_SIZE)
        {
            return;
        }

        char *tempBuff = malloc(size + pcm->size);
        if (tempBuff == NULL)
        {
            return;
        }

        memcpy(tempBuff, pcm->data, pcm->size);
        free(pcm->data);
        pcm->data = tempBuff;
        pcm->capacity = size + pcm->size;
    }

    // copy buffer to PCM
    memcpy(pcm->data + pcm->size, buff, size);
    pcm->size += size;
}

DEXPORT void DPCMReset(DPCM *pcm)
{
    if (pcm == NULL)
    {
        return;
    }

    pcm->size = 0;
}

DEXPORT void DPCMClean(DPCM *pcm)
{
    if (pcm == NULL)
    {
        return;
    }

    // clean buffer
    if (pcm->data != NULL)
    {
        free(pcm->data);
        pcm->data = NULL;
    }
    pcm->capacity = 0;
    pcm->size = 0;
}
