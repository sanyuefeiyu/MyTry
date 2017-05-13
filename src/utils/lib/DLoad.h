/*
 * @author Double
 * @since 2017/05/13
 */

#ifndef D_LOAD_H
#define D_LOAD_H

#include "DExport.h"

#ifdef __cplusplus
extern "C" {
#endif

DEXPORT void* DLoadOpen(const char *filePath);
DEXPORT void* DLoadGetSymbol(const void *hdl, const char *symbolName);
DEXPORT void DLoadClose(const void *hdl);

#ifdef __cplusplus
}
#endif

#endif /* D_LOAD_H */
