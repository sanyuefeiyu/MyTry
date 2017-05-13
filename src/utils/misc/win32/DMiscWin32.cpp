#include <Windows.h>
#include "DLog.h"
#include "DMisc.h"

#define TAG     "DMISC"

static LPSTR ConvertErrorCodeToString(DWORD ErrorCode)
{
    HLOCAL LocalAddress=NULL;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_IGNORE_INSERTS|FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,ErrorCode,0,(PTSTR)&LocalAddress,0,NULL);
    return (LPSTR)LocalAddress;
}

DEXPORT void DMiscPrintError()
{
    DWORD errorCode = GetLastError();
    DLog(DLOG_W, TAG, "lastError is %d, %s", errorCode, ConvertErrorCodeToString(errorCode));
}
