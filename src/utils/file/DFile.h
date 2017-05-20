/*
 * @author Double
 * @since 2016/08/21
 */

#ifndef D_FILE_H
#define D_FILE_H

#include "DExport.h"

#ifdef __cplusplus
extern "C" {
#endif

DEXPORT void DFileWrite(const char *path, const char *buff, int size);
DEXPORT void DFileFlush(const char *path);

#ifdef __cplusplus
}
#endif

#endif /* D_FILE_H */
