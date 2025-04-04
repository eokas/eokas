
#include "./os.h"
#include "./string.h"
#include <cstdlib>
#include <iostream>

#if _EOKAS_OS == _EOKAS_OS_WIN64 || _EOKAS_OS == _EOKAS_OS_WIN32
    #include <Windows.h>
    #include <psapi.h>
    #include <Wbemidl.h>
    #include <iphlpapi.h>
    #pragma comment(lib, "Iphlpapi.lib")
#else
    #include <unistd.h>
    #include <pwd.h>
    #include <ifaddrs.h>
    #include <sys/utsname.h>
    #include <sys/socket.h>
    #include <net/if_types.h>
    #include <net/if_dl.h>
    #include <net/ethernet.h>
    #if _EOKAS_OS == _EOKAS_OS_MACOS || _EOKAS_OS == _EOKAS_OS_IOS
        #include <mach/mach.h>
    #endif
#endif

namespace eokas {
    String OS::getSystemName() {
#if _EOKAS_OS == _EOKAS_OS_WIN64 || _EOKAS_OS == _EOKAS_OS_WIN32
        return "Windows";
#elif _EOKAS_OS == __EOKAS_OS_LINUX
        return "Linux";
#elif _EOKAS_OS == _EOKAS_OS_MACOS
        return "macOS";
#elif _EOKAS_OS == _EOKAS_OS_IOS
        return "iOS";
#elif _EOKAS_OS == _EOKAS_OS_ANDROID
        return "Android";

#else
        return "Unknown";
#endif
    }
    
    String OS::getSystemVersion() {
        return "";
    }
    
    String OS::getDeviceName() {
#if _EOKAS_OS == _EOKAS_OS_WIN64 || _EOKAS_OS == _EOKAS_OS_WIN32
        CHAR buffer[256] = {0};
        DWORD count = 0;
        if (!GetComputerNameA(buffer, &count))
            return "";
        return String{buffer, count};
#else
        char buffer[256] = {0};
        gethostname(buffer, 256);
        return String{buffer};
#endif
    }
    
    String OS::getDeviceModel() {
        return "";
    }
    
    u32_t OS::getCpuCount() {
#if _EOKAS_OS == _EOKAS_OS_WIN64 || _EOKAS_OS == _EOKAS_OS_WIN32
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        u32_t cpuCount = sysInfo.dwNumberOfProcessors;
        return cpuCount;
#else
        u32_t cpuCount = sysconf(_SC_NPROCESSORS_ONLN);
        return cpuCount;
#endif
    }
    
    String OS::getCpuArch() {
#if _EOKAS_ARCH == _EOKAS_ARCH_X86
        return "x86";
#elif _EOKAS_ARCH == _EOKAS_ARCH_X64
        return "x64";
#elif _EOKAS_ARCH == _EOKAS_ARCH_ARM32
        return "armv7";
#elif _EOKAS_ARCH == _EOKAS_ARCH_ARM64
        return "arm64";
#elif _EOKAS_ARCH == _EOKAS_ARCH_MIPS
        return "mips";
#elif _EOKAS_ARCH == _EOKAS_ARCH_SUPERH
        return "superh";
#elif _EOKAS_ARCH == _EOKAS_ARCH_POWERPC
        return "ppc";
#elif _EOKAS_ARCH == _EOKAS_ARCH_POWERPC64
        return "ppc64";
#elif _EOKAS_ARCH == _EOKAS_ARCH_SPARC
        return "sparc";
#elif _EOKAS_ARCH == _EOKAS_ARCH_M68K
        return "m68k";
else
        return "unknown";
#endif
    }
    
    void OS::getMemoryState(MemoryState& ms) {
#if _EOKAS_OS == _EOKAS_OS_WIN64 || _EOKAS_OS == _EOKAS_OS_WIN32
        MEMORYSTATUSEX memoryStatus;
        memoryStatus.dwLength = sizeof(MEMORYSTATUSEX);
        GlobalMemoryStatusEx(&memoryStatus);
        
        HANDLE hProcess = GetCurrentProcess();
        PROCESS_MEMORY_COUNTERS pmc;
        GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc));
        
        ms.total = memoryStatus.ullTotalPhys;
        ms.available = memoryStatus.ullAvailPhys;
        ms.process = pmc.WorkingSetSize;
#elif _EOKAS_OS == _EOKAS_OS_LINUX || _EOKAS_OS == _EOKAS_OS_ANDROID
        std::string line;
        
        std::ifstream meminfo("/proc/meminfo");
        while (std::getline(meminfo, line)) {
            if (line.find("MemTotal:") != std::string::npos) {
                sscanf(line.c_str(), "MemTotal: %llu", &ms.total);
            } else if (line.find("MemFree:") != std::string::npos) {
                sscanf(line.c_str(), "MemAvailable: %llu", &ms.available);
            }
            if(ms.total > 0 && ms.available > 0) {
                break;
            }
        }
    
        std::ifstream status("/proc/self/status");
        while (std::getline(status, line)) {
            if (line.find("VmRSS:") != std::string::npos) {
                sscanf(line.c_str(), "VmRSS:%llu", &ms.process);
            }
        }
#elif _EOKAS_OS == _EOKAS_OS_MACOS || _EOKAS_OS == _EOKAS_OS_IOS
        mach_port_t host_port = mach_host_self();
        mach_msg_type_number_t host_size = sizeof(vm_statistics_data_t) / sizeof(integer_t);
        vm_size_t pagesize;
        host_page_size(host_port, &pagesize);
        vm_statistics_data_t vm_stat;
    
        if (host_statistics(host_port, HOST_VM_INFO, (host_info_t)&vm_stat, &host_size) == KERN_SUCCESS) {
            natural_t mem_used = (vm_stat.active_count + vm_stat.inactive_count + vm_stat.wire_count) * pagesize;
            ms.total = mem_used + vm_stat.free_count * pagesize;
            ms.available = vm_stat.free_count * pagesize;
        }
    
        struct task_basic_info t_info;
        mach_msg_type_number_t t_info_count = TASK_BASIC_INFO_COUNT;
        if (KERN_SUCCESS == task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)&t_info, &t_info_count)) {
            ms.process = (u64_t)(t_info.resident_size);
            // ms.virtual = t_info.resident_size;
        }
#endif
    }
    
    String OS::getEnv(const String& name) {
        return std::getenv(name.cstr());
    }
    
    bool OS::setEnv(const String& name, const String& value) {
#if _EOKAS_OS == _EOKAS_OS_WIN64 || _EOKAS_OS == _EOKAS_OS_WIN32
        return _putenv_s(name.cstr(), value.cstr()) == 0;
#else
        return setenv(name.cstr(), value.cstr(), 1) == 0;
#endif
    }
    
    String OS::getCurrentUser() {
#if _EOKAS_OS == _EOKAS_OS_WIN64 || _EOKAS_OS == _EOKAS_OS_WIN32
        CHAR buffer[256];
        DWORD count = 0;
        if (!GetUserNameA(buffer, &count))
            return "";
        return String(buffer, count);
#else
        uid_t uid = geteuid();
        struct passwd* pw = getpwuid(uid);
        if (pw == nullptr)
            return "";
        return pw->pw_name;
#endif
    }
    
    String OS::getMacAddress() {
#if _EOKAS_OS == _EOKAS_OS_WIN64 || _EOKAS_OS == _EOKAS_OS_WIN32
        IP_ADAPTER_INFO adapterInfo[16];
        DWORD dwBufLen = sizeof(adapterInfo);
        if (GetAdaptersInfo(adapterInfo, &dwBufLen) != ERROR_SUCCESS)
            return "";
        String macAddr;
        PIP_ADAPTER_INFO adapterPtr = adapterInfo;
        while (adapterPtr != nullptr) {
            if(adapterPtr->Type == MIB_IF_TYPE_ETHERNET) {
                for (UINT i = 0; i < adapterPtr->AddressLength; i++) {
                    macAddr += String::format("%02X", adapterPtr->Address[i]);
                }
                break;
            }
            adapterPtr = adapterPtr->Next;
        }
        return macAddr;
#else
        struct ifaddrs* addrs;
        if (getifaddrs(&addrs) != 0) {
            return "";
        }
        
        String macAddress;
        for (struct ifaddrs* addrPtr = addrs; addrPtr != nullptr; addrPtr = addrPtr->ifa_next) {
            if(addrPtr->ifa_addr->sa_family == AF_LINK) {
                sockaddr_dl* sdl = (sockaddr_dl*)(addrPtr->ifa_addr);
                if(sdl->sdl_type == IFT_ETHER) {
                    char* mac = (char*)ether_ntoa((const struct ether_addr*)LLADDR(sdl));
                    macAddress = mac;
                    break;
                }
            }
        }
    
        freeifaddrs(addrs);
        
        return macAddress;
#endif
    }
}
