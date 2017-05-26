/*
 * @author Double
 * @since 2017/05/26
 */

#ifndef D_BIT_STREAM_H
#define D_BIT_STREAM_H

#include "DExport.h"

#ifdef __cplusplus
extern "C" {
#endif

DEXPORT void* DBitStreamInit(unsigned char *buf, unsigned int capacity);
DEXPORT int DBitStreamGetLeftSize(void *bs);
DEXPORT int DBitStreamReadChar(void *bs, unsigned char *ch);
DEXPORT int DBitStreamReadShort(void *bs, unsigned short *number);
DEXPORT int DBitStreamReadInteger(void *bs, unsigned int *number);
DEXPORT int DBitStreamReadBuf(void *bs, unsigned char *buf, unsigned int size);
DEXPORT void DBitStreamRelease(void **bs);

#ifdef __cplusplus
}
#endif

#endif /* D_BIT_STREAM_H */
