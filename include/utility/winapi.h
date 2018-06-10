#ifndef _MTS_UTILITY_WINAPI_H_INCLUDED
#define _MTS_UTILITY_WINAPI_H_INCLUDED

#include "config.h"

#ifdef _OS_WINDOWS
#include <wtypes.h>

namespace utility
{
    namespace winapi
    {
        using ::GetSystemInfo;
        using ::Sleep;
        using ::SwitchToThread;

        inline void sched_yield()
        {
            if (!SwitchToThread())
            {
                Sleep(0);
            }
        }

        inline void sleep_tick()
        {
            Sleep(1);
        }

        using ::CreateMutexA;
        using ::CloseHandle;
        using ::SetLastError;
        using ::GetLastError;
        using ::ReleaseMutex;

        struct errc
        {
            enum 
            {
                error_already_exists = ERROR_ALIAS_EXISTS
            };
        };
    }
}
#endif

#endif //_MTS_UTILITY_WINAPI_H_INCLUDED
