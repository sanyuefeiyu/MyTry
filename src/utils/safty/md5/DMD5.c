#include <stdlib.h>
#include <stdio.h>
#include "DLog.h"
#include "DMD5.h"
#include "./cifs/MD5.h"

#define MD5_BLOCK   64
#define READ_SIZE   (2*1024)
#define TAG "DMD5"

struct DMD5Ctx
{
    auth_md5Ctx cifs_ctx[1];
};

DMD5Ctx* DMD5InitCtx()
{
    DMD5Ctx *ctx = (DMD5Ctx*)malloc(sizeof(DMD5Ctx));
    if (ctx == NULL)
        return NULL;

    auth_md5InitCtx(ctx->cifs_ctx);
    return ctx;
}

void DMD5SumCtx(DMD5Ctx *ctx, const unsigned char *src, const int len)
{
    int consumedSize = 0;

    if (ctx == NULL || src == NULL || len <= 0)
        return;

    while (consumedSize < len)
    {
        int size = len - consumedSize;
        if (size >= MD5_BLOCK)
            size = MD5_BLOCK;

        auth_md5SumCtx(ctx->cifs_ctx, src + consumedSize, size);
        consumedSize += size;
    }
}

void DMD5CloseCtx(DMD5Ctx *ctx, unsigned char *dst)
{
    if (ctx == NULL || dst == NULL)
        return;

    auth_md5CloseCtx(ctx->cifs_ctx, dst );
}

void DMD5ReleaseCtx(DMD5Ctx **ctx)
{
    if (ctx == NULL || *ctx == NULL)
        return;

    free(*ctx);
}

void DMD5Sum(unsigned char *dst, const unsigned char *src, const int len)
{
    auth_md5Sum(dst, src, len);
}

void DMD5SumFile(unsigned char *dst, const char *filePath)
{
    long fileSize = 0;
    long readSize = 0;
    unsigned char src[READ_SIZE];
    DMD5Ctx *ctx = NULL;

    if (dst == NULL || filePath == NULL)
        return;

    FILE *fp = fopen(filePath, "rb");
    if (fp == NULL)
    {
        DLog(DLOG_W, TAG, "open file failed, filePath=%s", filePath);
        return;
    }
    fseek(fp, 0, SEEK_END);
    fileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (fileSize <= 0)
    {
        DLog(DLOG_W, TAG, "file size is 0, filePath=%s", filePath);
        fclose(fp);
        return;
    }

    ctx = DMD5InitCtx();
    if (ctx == NULL)
    {
        fclose(fp);
        return;
    }

    readSize = fread(src, 1, READ_SIZE, fp);
    while (readSize > 0)
    {
        DMD5SumCtx(ctx, src, readSize);
        readSize = fread(src, 1, READ_SIZE, fp);
    }

    DMD5CloseCtx(ctx, dst);
    DMD5ReleaseCtx(&ctx);
    fclose(fp);
}
