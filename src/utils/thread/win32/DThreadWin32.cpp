#include <stdlib.h>
#include <Windows.h>
#include "DMisc.h"
#include "DLog.h"
#include "DThread.h"

#define TAG     "DThread"

typedef struct
{
    CRITICAL_SECTION cs;
} DMutex;

DEXPORT void* DMutexInit()
{
    DMutex *dMutex = (DMutex*)malloc(sizeof(DMutex));
    if (dMutex == NULL)
        return NULL;

    InitializeCriticalSection(&dMutex->cs);

    return dMutex;
}

DEXPORT void DMutexRelease(void **mutex)
{
    if (mutex == NULL || *mutex == NULL)
        return;

    DMutex *dMutex = (DMutex*)(*mutex);
    free(dMutex);
    *mutex = NULL;
}

DEXPORT void DMutexLock(void *mutex)
{
    if (mutex == NULL)
        return;

    DMutex *dMutex = (DMutex*)mutex;
    EnterCriticalSection(&dMutex->cs);
}

DEXPORT void DMutexunLock(void *mutex)
{
    if (mutex == NULL)
        return;

    DMutex *dMutex = (DMutex*)mutex;
    LeaveCriticalSection(&dMutex->cs);
}

typedef struct
{
    CONDITION_VARIABLE cv;
} DConditionVarible;

DEXPORT void* DConditionVaribleInit()
{
    DConditionVarible *dCV = (DConditionVarible*)malloc(sizeof(DConditionVarible));
    if (dCV == NULL)
        return NULL;

    InitializeConditionVariable(&dCV->cv);

    return dCV;
}

DEXPORT void DConditionVaribleRelease(void **cv)
{
    if (cv == NULL || *cv == NULL)
        return;

    DConditionVarible *dCV = (DConditionVarible*)(*cv);
    free(dCV);
    *cv = NULL;
}

DEXPORT void DConditionVaribleWait(void *cv, void *mutex)
{
    if (cv == NULL || mutex == NULL)
        return;

    DConditionVarible *dCV = (DConditionVarible*)cv;
    DMutex *dMutex = (DMutex*)mutex;
    SleepConditionVariableCS(&dCV->cv, &dMutex->cs, INFINITE);
}

DEXPORT void DConditionVaribleSignal(void *cv)
{
    if (cv == NULL)
        return;

    DConditionVarible *dCV = (DConditionVarible*)cv;
    WakeConditionVariable(&dCV->cv);
}

DEXPORT void DSleep(unsigned int milliseconds)
{
    Sleep(milliseconds);
}
