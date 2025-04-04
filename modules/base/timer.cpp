
#include "timer.h"

#if _EOKAS_OS == _EOKAS_OS_WIN64 || _EOKAS_OS == _EOKAS_OS_WIN32

#include <windows.h>

namespace eokas {
    
    struct TimerImpl {
        f64_t lastCount;
    };
    
    Timer::Timer()
        : mImpl(new TimerImpl()) {
        this->reset();
    }
    
    Timer::~Timer() {
        delete mImpl;
    }
    
    void Timer::reset() {
        LARGE_INTEGER counter;
        QueryPerformanceCounter(&counter);
        mImpl->lastCount = (f64_t) (counter.QuadPart);
    }
    
    i64_t Timer::elapse(bool isReset) {
        LARGE_INTEGER count;
        QueryPerformanceCounter(&count);
        f64_t curCount = (f64_t) (count.QuadPart);
        f64_t lastCount = mImpl->lastCount;
        if (isReset) {
            mImpl->lastCount = curCount;
        }
        
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);
        f64_t curFrequency = (f64_t) (frequency.QuadPart);
        
        i64_t result = (i64_t) ((curCount - lastCount) / curFrequency * 1000000);
        return result;
    }
    
}

#elif _EOKAS_OS == _EOKAS_OS_MACOS || _EOKAS_OS == _EOKAS_OS_IOS

#include <mach/mach_time.h>

namespace eokas {

    struct TimerImpl
    {
        u64_t last;
    };
    
    Timer::Timer()
        :mImpl(new TimerImpl())
    {
        this->reset();
    }
    
    Timer::~Timer()
    {
        delete mImpl;
    }
    
    void Timer::reset()
    {
        mImpl->last = mach_absolute_time();
    }
    
    i64_t Timer::elapse(bool isReset)
    {
        u64_t now = mach_absolute_time();
        u64_t last = mImpl->last;
        if (isReset)
        {
            mImpl->last = now;
        }
    
        mach_timebase_info_data_t info;
        mach_timebase_info(&info);
        f64_t factor = f64_t(info.numer) / f64_t(info.denom) * 1e-9;
    
        i64_t result = i64_t((now - last) * factor * 1000000);
        return result;
    }
}

#endif
