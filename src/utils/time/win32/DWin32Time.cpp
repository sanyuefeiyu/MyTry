#include "windows.h"
#include "mmsystem.h"
#include "DTime.h"

long long DTimeGetTick()
{
	return timeGetTime();
}
