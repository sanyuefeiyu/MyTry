#include <stdlib.h>
#include <dlfcn.h>
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

    void *hDllLib = dlopen(path, RTLD_LAZY);
    DLog(DLOG_D, TAG, "DLoadOpen %s and result is %p", path, hDllLib);
    if (hDllLib == NULL)
    {
        DMiscPrintError();
    }

    return hDllLib;
}

DEXPORT void* DLoadGetSymbol(const void *hdl, const char *symbolName)
{
    if (hdl == NULL || symbolName == NULL)
    {
        return NULL;
    }

    void *proc = dlsym((void*)hdl, symbolName);
    DLog(DLOG_D, TAG, "DLoadGetSymbol %p, %s and result is %p", hdl, symbolName, proc);
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
    dlclose((void*)hdl);
}
