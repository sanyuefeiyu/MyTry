#include <stdlib.h>
#include <Windows.h>
#include <list>
#include "Mmsystem.h"
#include "DMisc.h"
#include "DThread.h"
#include "DAudioOut.h"
#include "DTime.h"
#include "DLog.h"

#define TAG                 "DAO"
#define MAX_BUFFER_COUNT    10

using namespace std;

typedef enum
{
    AS_NONE,
    AS_OPENING,
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
    bool wait;
    AudioState as;
    int bufferCount;

    list<LPWAVEHDR> *waveHDRList;
} DAO;

static void Add2List(DAO *dAO, LPWAVEHDR waveHDR)
{
    dAO->waveHDRList->push_back(waveHDR);
}

static void CleanList(DAO *dAO)
{
    while (dAO->waveHDRList->size() > 0)
    {
        LPWAVEHDR waveHDR = *dAO->waveHDRList->begin();
        waveOutUnprepareHeader(dAO->hWaveOut, waveHDR, sizeof(WAVEHDR));
        free(waveHDR->lpData);
        free(waveHDR);

        dAO->waveHDRList->pop_front();
    }
}

static void CALLBACK WaveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2 )
{
    if (dwInstance == NULL)
        return;

    DAO *dAO = (DAO*)dwInstance;

    DMutexLock(dAO->mutex);

    switch (uMsg)
    {
    case WOM_OPEN:
        dAO->as = AS_OPENED;
        if (dAO->wait)
            DConditionVaribleSignal(dAO->cv);
        DLog(DLOG_D, TAG, "WaveOutProc, after uMsg=%#X, WOM_OPEN", uMsg);
        break;
    case WOM_CLOSE:
        DLog(DLOG_D, TAG, "WaveOutProc, uMsg=%#X, WOM_CLOSE", uMsg);
        dAO->as = AS_CLOSED;
        break;
    case WOM_DONE:
    {
        long long start = DTimeGetTick();
        Add2List(dAO, (LPWAVEHDR)dwParam1);
        dAO->bufferCount--;
        if (dAO->wait)
            DConditionVaribleSignal(dAO->cv);
        DLog(DLOG_D, TAG, "WaveOutProc, after uMsg=%#X, WOM_DONE, bufferCount=%d, diff=%lld", uMsg, dAO->bufferCount, DTimeGetTick() - start);
        break;
    }
    default:
        DLog(DLOG_D, TAG, "WaveOutProc, uMsg=%#X, unknown", uMsg);
        break;
    }

    DMutexunLock(dAO->mutex);

    return;
}

static int WaveOutOpen(DAO *dAO, AudioAttr *audioAttr)
{
    DLog(DLOG_D, TAG, "WaveOutOpen");

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

    DMutexLock(dAO->mutex);
    dAO->as = AS_OPENING;
    if (waveOutOpen(&dAO->hWaveOut, WAVE_MAPPER, &wfx, (DWORD)&WaveOutProc, (DWORD)dAO, CALLBACK_FUNCTION))
    {
        DMiscPrintError();
        DMutexunLock(dAO->mutex);
        return -1;
    }
    if (dAO->as == AS_OPENING)
    {
        long long start = DTimeGetTick();
        dAO->wait = true;
        DConditionVaribleWait(dAO->cv, dAO->mutex);
        dAO->wait = false;
        DLog(DLOG_D, TAG, "after wait for open, diff=%lld", DTimeGetTick() - start);
    }
    DMutexunLock(dAO->mutex);
    return 0;
}

static int WaveOutWrite(DAO *dAO, DPCM *pcm)
{
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

    DLog(DLOG_D, TAG, "waveOutWrite, size=%u", pcm->size);
    if (waveOutWrite(dAO->hWaveOut, pWaveHeader, sizeof(WAVEHDR)))
    {
        free(pWaveHeader->lpData);
        free(pWaveHeader);
        DMutexunLock(dAO->mutex);
        return -1;
    }

    DMutexLock(dAO->mutex);
    CleanList(dAO);
    dAO->bufferCount++;
    if (dAO->bufferCount >= MAX_BUFFER_COUNT)
    {
        long long start = DTimeGetTick();
        dAO->wait = true;
        DConditionVaribleWait(dAO->cv, dAO->mutex);
        dAO->wait = false;
        DLog(DLOG_D, TAG, "waveOutWrite, after wait and bufferCount=%d, diff=%lld", dAO->bufferCount, DTimeGetTick() - start);
    }
    DMutexunLock(dAO->mutex);

    return 0;
}

static void WaveOutClose(DAO *dAO)
{
    while (1)
    {
        if (dAO->bufferCount <= 0)
            break;

        DSleep(10);
    }

    DMutexLock(dAO->mutex);
    waveOutReset(dAO->hWaveOut);
    CleanList(dAO);
    waveOutClose(dAO->hWaveOut);
    DMutexunLock(dAO->mutex);
}

DEXPORT void* DAOInit()
{
    DLog(DLOG_D, TAG, "DAOInit");

    DAO *dAO = (DAO*)calloc(1, sizeof(DAO));

    dAO->mutex = DMutexInit();
    dAO->cv = DConditionVaribleInit();
    dAO->wait = false;
    dAO->as = AS_NONE;
    dAO->bufferCount = 0;
    dAO->waveHDRList = new list<LPWAVEHDR>;

    return dAO;
}

DEXPORT void DAORelease(void **ao)
{
    DLog(DLOG_D, TAG, "DAORelease");

    if (ao == NULL || *ao == NULL)
        return;

    DAO *dAO = (DAO*)(*ao);

    DMutexRelease(&dAO->mutex);
    DConditionVaribleRelease(&dAO->cv);
    delete dAO->waveHDRList;
    free(dAO);
    *ao = NULL;
}

DEXPORT int DAOOpen(void *ao, AudioAttr *audioAttr)
{
    DLog(DLOG_D, TAG, "DAOOpen");

    if (ao == NULL || audioAttr == NULL)
        return -1;

    DAO *dAO = (DAO*)ao;
    DMutexLock(dAO->mutex);
    if (dAO->as == AS_OPENING)
    {
        DMutexunLock(dAO->mutex);
        return -1;
    }
    DMutexunLock(dAO->mutex);

    return WaveOutOpen(dAO, audioAttr);
}

DEXPORT int DAOWrite(void *ao, DPCM *pcm)
{
    if (ao == NULL || pcm == NULL)
        return -1;

    DAO *dAO = (DAO*)ao;
    if (pcm->data == NULL || pcm->size <= 0)
        return -1;

    return WaveOutWrite(dAO, pcm);
}

DEXPORT void DAOClose(void *ao)
{
    DLog(DLOG_D, TAG, "DAOClose");

    if (ao == NULL)
        return;

    DAO *dAO = (DAO*)ao;
    WaveOutClose(dAO);
}
