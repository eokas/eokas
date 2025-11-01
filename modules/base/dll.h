#pragma once
#include "./header.h"
#include "./string.h"

namespace eokas {
    
    class Dll {
        _ForbidCopy(Dll);
        _ForbidAssign(Dll);
    
    public:
        static const char* extension;
        static void* dllopen(const char* file);
        static void dllclose(void* handle);
        static void* dllsymbol(void* handle, const char* name);
    
    public:
        Dll(const String& name);
        ~Dll();
    
    public:
        bool open();
        void close();
        bool isOpen() const;
        const String& name() const;
        void* getSymbol(const char* symbolName) const;
    
    private:
        String mName;
        void* mHandle;
    };
    
}
