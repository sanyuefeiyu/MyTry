#include <string.h>
#include <errno.h>
#include "DLog.h"
#include "DMisc.h"

#define TAG     "DMisc"

DEXPORT void DMiscPrintError()
{
	int errorCode = errno;
    DLog(DLOG_W, TAG, "lastError is %d, %s", errorCode, strerror(errorCode));
}
