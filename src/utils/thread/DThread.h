/*
 * @author Double
 * @since 2017/06/10
 */

#ifndef D_THREAD_H
#define D_THREAD_H

#include "DExport.h"

#ifdef __cplusplus
extern "C" {
#endif

DEXPORT void* DMutexInit();
DEXPORT void DMutexRelease(void **mutex);
DEXPORT void DMutexLock(void *mutex);
DEXPORT void DMutexunLock(void *mutex);

DEXPORT void* DSemaphoreInit();
DEXPORT void DSemaphoreRelease(void **semaphore);
DEXPORT void DSemaphoreWait(void *semaphore);
DEXPORT void DSemaphoreSignal(void *semaphore);

DEXPORT void DSleep(unsigned int milliseconds);

#ifdef __cplusplus
}
#endif

#endif /* D_THREAD_H */
