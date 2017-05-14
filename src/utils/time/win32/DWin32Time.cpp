#include "windows.h"
#include "mmsystem.h"
#include "DTime.h"

DEXPORT long long DTimeGetTick()
{
    return timeGetTime();
}
