/*
 * @author Double
 * @since 2016/04/17
 */

#ifndef D_LOG_H
#define D_LOG_H

#include "DExport.h"

typedef enum
{
    DLOG_D = 0,
    DLOG_I,
    DLOG_W,
    DLOG_E
} DLogLevel;

typedef enum
{
    DLOG_CONSOLE = 0x1,
    DLOG_FILE = 0x1 << 1
} DLogMode;

#ifdef __cplusplus
extern "C" {
#endif

DEXPORT void DLogFlush();
DEXPORT void DLogOutputMode(DLogMode logMode);
DEXPORT void DLog(DLogLevel level, const char *tag, const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif /* D_LOG_H */
