/*
 * @author Double
 * @since 2017/06/10
 */

#ifndef D_THREAD_H
#define D_THREAD_H

#include "DExport.h"

typedef int (*DThreadProc)(void *param);

#ifdef __cplusplus
extern "C" {
#endif

DEXPORT void* DThreadInit(DThreadProc proc, void *param);
DEXPORT void DThreadJoin(void *thread);
DEXPORT void DThreadRelease(void **thread);

DEXPORT void* DMutexInit();
DEXPORT void DMutexRelease(void **mutex);
DEXPORT void DMutexLock(void *mutex);
DEXPORT void DMutexunLock(void *mutex);

DEXPORT void* DConditionVaribleInit();
DEXPORT void DConditionVaribleRelease(void **cv);
DEXPORT void DConditionVaribleWait(void *cv, void *mutex);
DEXPORT void DConditionVaribleSignal(void *cv);

DEXPORT void DSleep(unsigned int milliseconds);

#ifdef __cplusplus
}
#endif

#endif /* D_THREAD_H */
