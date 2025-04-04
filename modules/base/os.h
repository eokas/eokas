
#ifndef _EOKAS_BASE_OS_H_
#define _EOKAS_BASE_OS_H_

#include "./header.h"

namespace eokas {
    struct MemoryState {
        u64_t total = 0;
        u64_t available = 0;
        u64_t process = 0;
    };
    
    struct OS {
        static String getSystemName();
        static String getSystemVersion();
        static String getDeviceName();
        static String getDeviceModel();
        static u32_t getCpuCount();
        static String getCpuArch();
        static void getMemoryState(MemoryState& ms);
        static String getEnv(const String& name);
        static bool setEnv(const String& name, const String& value);
        static String getCurrentUser();
        static String getMacAddress();
    };
}

#endif //_EOKAS_BASE_OS_H_
