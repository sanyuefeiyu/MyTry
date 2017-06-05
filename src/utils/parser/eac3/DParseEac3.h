/*
 * @author Double
 * @since 2017/06/04
 */

#ifndef D_PARSE_EAC3_H
#define D_PARSE_EAC3_H

#include "DExport.h"

typedef struct
{
    unsigned short syncword; // 16
    unsigned short crc1; // 16
    unsigned char fscod; // 2
    unsigned char frmsizecod; // 6
} SyncInfo;

typedef struct
{
    SyncInfo syncInfo;

    int sampleRate;
    int frameSize;
    int bitrate;

    int startPos;
} SyncFrame;

#ifdef __cplusplus
extern "C" {
#endif

DEXPORT int DParseEac3(const unsigned char *buf, int size, SyncFrame *frame);

#ifdef __cplusplus
}
#endif

#endif /* D_PARSE_EAC3_H */
