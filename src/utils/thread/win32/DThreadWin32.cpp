#include <stdlib.h>
#include <Windows.h>
#include "DMisc.h"
#include "DLog.h"
#include "DThread.h"

#define TAG     "DThread"

typedef struct
{
    HANDLE hMutex;
} DMutex;

DEXPORT void* DMutexInit()
{
    DMutex *dMutex = (DMutex*)malloc(sizeof(DMutex));
    if (dMutex == NULL)
        return NULL;

    dMutex->hMutex = CreateMutex(NULL, FALSE, NULL);
    if (dMutex->hMutex == NULL)
    {
        DMiscPrintError();
        free(dMutex);
        return NULL;
    }

    return dMutex;
}

DEXPORT void DMutexRelease(void **mutex)
{
    if (mutex == NULL || *mutex == NULL)
        return;

    DMutex *dMutex = (DMutex*)(*mutex);
    CloseHandle(dMutex->hMutex);
    free(dMutex);
    *mutex = NULL;
}

DEXPORT void DMutexLock(void *mutex)
{
    if (mutex == NULL)
        return;

    DMutex *dMutex = (DMutex*)mutex;
    WaitForSingleObject(dMutex->hMutex, INFINITE);
}

DEXPORT void DMutexunLock(void *mutex)
{
    if (mutex == NULL)
        return;

    DMutex *dMutex = (DMutex*)mutex;
    ReleaseMutex(dMutex->hMutex);
}

typedef struct
{
    HANDLE hSemaphore;
} DSemaphore;

DEXPORT void* DSemaphoreInit()
{
    DSemaphore *dSemaphore = (DSemaphore*)malloc(sizeof(DSemaphore));
    if (dSemaphore == NULL)
        return NULL;

    dSemaphore->hSemaphore = CreateSemaphore(NULL, 0, 1, NULL);
    if (dSemaphore->hSemaphore == NULL)
    {
        DMiscPrintError();
        free(dSemaphore);
        return NULL;
    }

    return dSemaphore;
}

DEXPORT void DSemaphoreRelease(void **semaphore)
{
    if (semaphore == NULL || *semaphore == NULL)
        return;

    DSemaphore *dSemaphore = (DSemaphore*)(*semaphore);
    CloseHandle(dSemaphore->hSemaphore);
    free(dSemaphore);
    *semaphore = NULL;
}

DEXPORT void DSemaphoreWait(void *semaphore)
{
    if (semaphore == NULL)
        return;

    DSemaphore *dSemaphore = (DSemaphore*)semaphore;
    WaitForSingleObject(dSemaphore->hSemaphore, INFINITE);
}

DEXPORT void DSemaphoreSignal(void *semaphore)
{
    if (semaphore == NULL)
        return;

    DSemaphore *dSemaphore = (DSemaphore*)semaphore;
    ReleaseSemaphore(dSemaphore->hSemaphore, 1, NULL);
}

DEXPORT void DSleep(unsigned int milliseconds)
{
    Sleep(milliseconds);
}
