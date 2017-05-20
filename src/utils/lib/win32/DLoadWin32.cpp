#include <Windows.h>
#include "DMisc.h"
#include "DLog.h"
#include "DLoad.h"

#define TAG     "DLoad"

DEXPORT void* DLoadOpen(const char *path)
{
    if (path == NULL)
    {
        return NULL;
    }

    HMODULE hDllLib = LoadLibrary(path);
    DLog(DLOG_D, TAG, "DLoadOpen %s and result is %p", path, hDllLib);
    if (hDllLib == NULL)
    {
        DMiscPrintError();
    }

    return hDllLib;
}

DEXPORT void* DLoadGetSymbol(const void *hdl, const char *symbol)
{
    if (hdl == NULL || symbol == NULL)
    {
        return NULL;
    }

    FARPROC proc = GetProcAddress((HMODULE)hdl, symbol);
    DLog(DLOG_D, TAG, "DLoadGetSymbol %p, %s and result is %p", hdl, symbol, proc);
    if (proc == NULL)
    {
        DMiscPrintError();
    }

    return proc;
}

DEXPORT void DLoadClose(const void *hdl)
{
    if (hdl == NULL)
    {
        return;
    }

    DLog(DLOG_D, TAG, "DLoadClose %p", hdl);
    FreeLibrary((HMODULE)hdl);
}
