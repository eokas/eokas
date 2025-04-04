
#include "dll.h"
#include "string.h"

#if _EOKAS_OS == _EOKAS_OS_WIN64 || _EOKAS_OS == _EOKAS_OS_WIN32
#include <Windows.h>
#elif _EOKAS_OS == _EOKAS_OS_MACOS || _EOKAS_OS == _EOKAS_OS_IOS
#include <dlfcn.h>
#endif

namespace eokas {

#if _EOKAS_OS == _EOKAS_OS_WIN64 || _EOKAS_OS == _EOKAS_OS_WIN32
    const char* Dll::extension = ".dll";
#elif _EOKAS_OS == _EOKAS_OS_MACOS || _EOKAS_OS == _EOKAS_OS_IOS
    const char* Dll::extension = ".dylib";
#endif
    
    void* Dll::dllopen(const char* file) {
#if _EOKAS_OS == _EOKAS_OS_WIN64 || _EOKAS_OS == _EOKAS_OS_WIN32
        return LoadLibraryA(file);
#elif _EOKAS_OS == _EOKAS_OS_MACOS || _EOKAS_OS == _EOKAS_OS_IOS
        return dlopen(file, RTLD_LAZY);
#endif
    }
    
    void Dll::dllclose(void* handle) {
#if _EOKAS_OS == _EOKAS_OS_WIN64 || _EOKAS_OS == _EOKAS_OS_WIN32
        FreeLibrary((HMODULE) handle);
#elif _EOKAS_OS == _EOKAS_OS_MACOS || _EOKAS_OS == _EOKAS_OS_IOS
        dlclose(handle);
#endif
    }
    
    void* Dll::dllsymbol(void* handle, const char* name) {
#if _EOKAS_OS == _EOKAS_OS_WIN64 || _EOKAS_OS == _EOKAS_OS_WIN32
        return (void*) GetProcAddress((HMODULE) handle, name);
#elif _EOKAS_OS == _EOKAS_OS_MACOS || _EOKAS_OS == _EOKAS_OS_IOS
        return dlsym(handle, name);
#endif
    }
    
    Dll::Dll(const String& name)
        : mName(name), mHandle(nullptr) {
    }
    
    Dll::~Dll() {
        if (mHandle != nullptr) {
            this->close();
            mHandle = nullptr;
        }
    }
    
    bool Dll::open() {
        String fileName = mName + Dll::extension;
        mHandle = Dll::dllopen(fileName.cstr());
        return mHandle != nullptr;
    }
    
    void Dll::close() {
        if (mHandle == nullptr)
            return;
        Dll::dllclose(mHandle);
        mHandle = nullptr;
    }
    
    bool Dll::isOpen() const {
        return mHandle != nullptr;
    }
    
    const String& Dll::name() const {
        return mName;
    }
    
    void* Dll::getSymbol(const char* symbolName) const {
        return Dll::dllsymbol(mHandle, symbolName);
    }
    
}
