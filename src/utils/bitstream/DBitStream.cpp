#include <stdlib.h>
#include <string.h>
#include "DBitStream.h"
#include "DLog.h"

#define TAG     "DFile"

typedef struct DBS
{
    unsigned char *buf;
    unsigned int capacity;
    unsigned int pos;
    int isBigEndian;
} DBS;

DEXPORT void* DBitStreamInit(unsigned char *buf, unsigned int capacity)
{
    if (buf == NULL || capacity <= 0)
        return NULL;

    DBS *bs = (DBS*)calloc(1, sizeof(DBS));
    if (bs == NULL)
        return NULL;

    bs->buf = buf;
    bs->capacity = capacity;
    bs->pos = 0;

    unsigned int test = 0x12345678;
    unsigned char *temp = (unsigned char*)&test;
    if (*temp == 0x78)
        bs->isBigEndian = 0;
    else
        bs->isBigEndian = 1;

    return bs;
}

DEXPORT int DBitStreamReInit(void *bs, unsigned char *buf, unsigned int capacity)
{
    if (bs == NULL || buf == NULL || capacity <= 0)
        return -1;

    DBS *hdl = (DBS*)bs;
    hdl->buf = buf;
    hdl->capacity = capacity;
    hdl->pos = 0;

    return 0;
}

DEXPORT int DBitStreamGetLeftSize(void *bs)
{
    if (bs == NULL)
        return -1;

    DBS *hdl = (DBS*)bs;

    return hdl->capacity - hdl->pos;
}

DEXPORT int DBitStreamGetPos(void *bs)
{
    if (bs == NULL)
        return -1;

    DBS *hdl = (DBS*)bs;

    return hdl->pos;
}

DEXPORT int DBitStreamReadChar(void *bs, unsigned char *ch)
{
    if (bs == NULL || ch == NULL)
        return -1;

    DBS *hdl = (DBS*)bs;
    if (hdl->pos >= hdl->capacity)
        return -1;

    *ch = hdl->buf[hdl->pos];
    hdl->pos++;

    return 0;
}

DEXPORT int DBitStreamReadShort(void *bs, unsigned short *number)
{
    if (bs == NULL || number == NULL)
        return -1;

    DBS *hdl = (DBS*)bs;
    if (hdl->pos + 1 >= hdl->capacity)
        return -1;

    short ret = 0;
    unsigned char *temp = (unsigned char*)&ret;
    if (hdl->isBigEndian)
    {
        *(temp + 1) = hdl->buf[hdl->pos + 0];
        *(temp + 0) = hdl->buf[hdl->pos + 1];
    }
    else
    {
        *(temp + 0) = hdl->buf[hdl->pos + 0];
        *(temp + 1) = hdl->buf[hdl->pos + 1];
    }

    hdl->pos += 2;
    *number = ret;

    return 0;
}

DEXPORT int DBitStreamReadInteger(void *bs, unsigned int *number)
{
    if (bs == NULL || number == NULL)
        return -1;

    DBS *hdl = (DBS*)bs;
    if (hdl->pos + 3 >= hdl->capacity)
        return -1;

    unsigned int ret = 0;
    unsigned char *temp = (unsigned char*)&ret;
    if (hdl->isBigEndian)
    {
        *(temp + 3) = hdl->buf[hdl->pos + 0];
        *(temp + 2) = hdl->buf[hdl->pos + 1];
        *(temp + 1) = hdl->buf[hdl->pos + 2];
        *(temp + 0) = hdl->buf[hdl->pos + 3];
    }
    else
    {
        *(temp + 0) = hdl->buf[hdl->pos + 0];
        *(temp + 1) = hdl->buf[hdl->pos + 1];
        *(temp + 2) = hdl->buf[hdl->pos + 2];
        *(temp + 3) = hdl->buf[hdl->pos + 3];
    }

    hdl->pos += 4;
    *number = ret;

    return 0;
}

DEXPORT int DBitStreamReadBuf(void *bs, unsigned char *buf, unsigned int size)
{
    if (bs == NULL || buf == NULL || size <= 0)
        return -1;

    DBS *hdl = (DBS*)bs;
    if (hdl->pos + size -1 >= hdl->capacity)
        return -1;

    memcpy(buf, hdl->buf + hdl->pos, size);
    hdl->pos += size;
    return 0;
}

DEXPORT void DBitStreamRelease(void **bs)
{
    if (bs == NULL || *bs == NULL)
        return;

    free(*bs);
    *bs = NULL;
}
