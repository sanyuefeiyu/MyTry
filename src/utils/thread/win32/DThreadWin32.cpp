#include <stdlib.h>
#include <Windows.h>
#include "DMisc.h"
#include "DLog.h"
#include "DThread.h"

#define TAG     "DThread"

typedef struct
{
    DWORD id;
    HANDLE hThread;

    DThreadProc proc;
    void *param;
} DThread;

static DWORD WINAPI ThreadProc(PVOID p)
{
    if (p == NULL)
        return -1;

    DThread *dThread = (DThread*)p;
    return dThread->proc(dThread->param);
}

DEXPORT void* DThreadInit(DThreadProc proc, void *param)
{
    if (proc == NULL)
        return NULL;

    DThread *dThread = (DThread*)calloc(1, sizeof(DThread));
    if (dThread == NULL)
        return NULL;

    dThread->proc = proc;
    dThread->param = param;
    dThread->hThread = CreateThread (NULL, 0, ThreadProc, (PVOID)dThread, 0, &dThread->id);

    return dThread;
}

DEXPORT void DThreadJoin(void *thread)
{
    if (thread == NULL)
        return;

    DThread *dThread = (DThread*)thread;
    WaitForSingleObject(dThread->hThread, INFINITE);
}

DEXPORT void DThreadRelease(void **thread)
{
    if (thread == NULL || *thread == NULL)
        return;

    DThread *dThread = (DThread*)(*thread);
    CloseHandle(dThread->hThread);
    free(dThread);
    *thread = NULL;

}

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
