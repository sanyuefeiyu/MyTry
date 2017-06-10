#include <stdlib.h>
#include <Windows.h>
#include "Mmsystem.h"
#include "DMisc.h"
#include "DThread.h"
#include "DAudioOut.h"
#include "DLog.h"

#define TAG     "DAO"

typedef enum
{
    AS_NONE,
    AS_OPENED,
    AS_CLOSED
} AudioState;

typedef struct
{
    HWAVEOUT hWaveOut;
    AudioAttr audioAttr;
    DPCM pcm;

    void *mutex;
    void *cv;
    AudioState as;
    int bufferCount;
} DAO;

static void CALLBACK WaveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2 )
{
    if (dwInstance == NULL)
        return;

    DAO *dAO = (DAO*)dwInstance;

    DMutexLock(dAO->mutex);

    switch (uMsg)
    {
    case WOM_OPEN:
        DLog(DLOG_D, TAG, "WaveOutProc, uMsg=%#X, WOM_OPEN", uMsg);
        dAO->as = AS_OPENED;
        break;
    case WOM_CLOSE:
        DLog(DLOG_D, TAG, "WaveOutProc, uMsg=%#X, WOM_CLOSE", uMsg);
        dAO->as = AS_CLOSED;
        break;
    case WOM_DONE:
        DLog(DLOG_D, TAG, "WaveOutProc, uMsg=%#X, WOM_DONE", uMsg);
        dAO->bufferCount--;
        DConditionVaribleSignal(dAO->cv);
        break;
    default:
        DLog(DLOG_D, TAG, "WaveOutProc, uMsg=%#X, unknown", uMsg);
        break;
    }

    DMutexunLock(dAO->mutex);

    return;
}

DEXPORT void* DAOInit()
{
    DAO *dAO = (DAO*)malloc(sizeof(DAO));

    dAO->mutex = DMutexInit();
    dAO->cv = DConditionVaribleInit();
    dAO->as = AS_NONE;
    dAO->bufferCount = 0;

    return dAO;
}

DEXPORT void DAORelease(void **ao)
{
    if (ao == NULL || *ao == NULL)
        return;

    DAO *dAO = (DAO*)(*ao);

    DMutexRelease(&dAO->mutex);
    DConditionVaribleRelease(&dAO->cv);
}

DEXPORT int DAOOpen(void *ao, AudioAttr *audioAttr)
{
    if (ao == NULL || audioAttr == NULL)
        return -1;

    DAO *dAO = (DAO*)ao;

    WAVEFORMATEX wfx;
    ZeroMemory(&wfx,sizeof(WAVEFORMATEX));

    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = audioAttr->channels;
    wfx.nSamplesPerSec = audioAttr->sampleRate;
    wfx.wBitsPerSample = audioAttr->sampleBits;
    wfx.nAvgBytesPerSec = wfx.nChannels * wfx.nSamplesPerSec * wfx.wBitsPerSample / 8;
    wfx.nBlockAlign = wfx.wBitsPerSample * wfx.nChannels / 8;
    wfx.cbSize = 0;

    if(waveOutOpen(0, 0, &wfx, 0, 0, WAVE_FORMAT_QUERY))
    {
        DMiscPrintError();
        return -1;
    }

    if (waveOutOpen(&dAO->hWaveOut, WAVE_MAPPER, &wfx, (DWORD)&WaveOutProc, (DWORD)dAO, CALLBACK_FUNCTION))
    {
        DMiscPrintError();
        return -1;
    }

    return 0;
}

DEXPORT int DAOWrite(void *ao, DPCM *pcm)
{
    if (ao == NULL || pcm == NULL)
        return -1;

    DAO *dAO = (DAO*)ao;

    LPWAVEHDR pWaveHeader = (LPWAVEHDR)malloc(sizeof(WAVEHDR));
    memset(pWaveHeader, 0, sizeof(WAVEHDR));
    pWaveHeader->lpData = (LPSTR)malloc(pcm->size);
    pWaveHeader->dwBufferLength = pcm->size;
    pWaveHeader->dwLoops = 1;

    if (!pWaveHeader->lpData)
    {
        free(pWaveHeader);
        return -1;
    }
    memcpy(pWaveHeader->lpData, pcm->data, pcm->size);
 
    if (waveOutPrepareHeader(dAO->hWaveOut, pWaveHeader, sizeof(WAVEHDR)))
    {
        free(pWaveHeader->lpData);
        free(pWaveHeader);
        return -1;
    }

    DMutexLock(dAO->mutex);

    DLog(DLOG_D, TAG, "waveOutWrite");
    if (waveOutWrite(dAO->hWaveOut, pWaveHeader, sizeof(WAVEHDR)))
    {
        free(pWaveHeader->lpData);
        free(pWaveHeader);
        DMutexunLock(dAO->mutex);
        return -1;
    }
    dAO->bufferCount++;
    if (dAO->bufferCount > 3)
    {
        DConditionVaribleWait(dAO->cv, dAO->mutex);
    }
    DMutexunLock(dAO->mutex);

    return 0;
}

DEXPORT void DAOClose(void *ao)
{

}
