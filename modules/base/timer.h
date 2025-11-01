#pragma once
#include "header.h"

namespace eokas {
    
    class Timer {
        _ForbidCopy(Timer);
        _ForbidAssign(Timer);
    
    public:
        Timer();
        ~Timer();
    
    public:
        void reset();
        i64_t elapse(bool isReset = true);
    
    private:
        struct TimerImpl* mImpl;
    };
    
}
