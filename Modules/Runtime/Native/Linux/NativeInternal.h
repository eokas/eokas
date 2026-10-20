#pragma once

#include <X11/Xlib.h>

#include <mutex>

namespace eokas
{
    inline int ignoreXError(Display*, XErrorEvent*)
    {
        return 0;
    }

    // XInitThreads must run before any other Xlib call in the process.
    inline void ensureXThreads()
    {
        static std::once_flag once;
        std::call_once(once, [] {
            XInitThreads();
            XSetErrorHandler(&ignoreXError);
        });
    }
}
