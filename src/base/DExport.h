/*
 * @author Double
 * @since 2016/04/19
 */

#ifndef D_EXPORT_H
#define D_EXPORT_H

#if defined(_WIN32)
    #define DEXPORT __declspec(dllexport)
#elif defined(__ANDROID__)
    #define DEXPORT __attribute__ ((visibility("default")))
#else
    #define DEXPORT
#endif

#endif /* D_EXPORT_H */
