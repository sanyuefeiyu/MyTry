/*
 * @author Double
 * @since 2016/08/25
 */

#ifndef D_MD5_H
#define D_MD5_H

#include "DExport.h"

typedef struct DMD5Ctx DMD5Ctx;

#ifdef __cplusplus
extern "C" {
#endif

DEXPORT DMD5Ctx* DMD5InitCtx();
DEXPORT void DMD5SumCtx(DMD5Ctx *ctx, const unsigned char *src, const int len);
DEXPORT void DMD5CloseCtx(DMD5Ctx *ctx, unsigned char *dst );
DEXPORT void DMD5ReleaseCtx(DMD5Ctx **ctx);
DEXPORT void DMD5Sum(unsigned char *dst, const unsigned char *src, const int len);
DEXPORT void DMD5SumFile(unsigned char *dst, const char *filePath);

#ifdef __cplusplus
}
#endif

#endif /* D_MD5_H */
