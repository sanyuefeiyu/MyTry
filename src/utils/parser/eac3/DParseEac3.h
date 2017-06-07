/*
 * @author Double
 * @since 2017/06/04
 */

#ifndef D_PARSE_EAC3_H
#define D_PARSE_EAC3_H

#include "DExport.h"

typedef struct
{
    unsigned char syncword[2]; // 16
    unsigned char crc1[2]; // 16
    unsigned char fscod; // 2
    unsigned char frmsizecod; // 6
} SyncInfo;

typedef struct
{
    unsigned char bsid; // 5
} AC3BSI;

typedef struct
{
    unsigned char strmtyp; // 2
    unsigned char substreamid; // 3
    unsigned short frmsiz; // 11
    unsigned char fscod; // 2
    unsigned char fscod2, numblkscod; // 2
    unsigned char acmod; // 3
    unsigned char ifeon; // 1
    unsigned char bsid; // 5
} eAC3BSI;

typedef struct
{
    SyncInfo syncInfo;
    AC3BSI bsi;
    eAC3BSI ebsi;

    int sampleRate;
    int frameSize;
    int bitrate;
    int numberBlock;

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
