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

    return dAO;
}

DEXPORT void DAORelease(void **ao)
{
    if (ao == NULL || *ao == NULL)
        return;

    DAO *dAO = (DAO*)ao;

    DMutexRelease(&dAO->mutex);
    DConditionVaribleRelease(&dAO->cv);
}

DEXPORT void DAOOpen(void *ao)
{

}

DEXPORT void DAWrite(void *ao, DPCM *pcm)
{

}

DEXPORT void DAOClose(void *ao)
{

}
