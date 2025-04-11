#ifndef _EOKAS_NATIVE_PASTEBOARD_H_
#define _EOKAS_NATIVE_PASTEBOARD_H_

#include "./header.h"

namespace eokas::native
{
    class PasteBoard
    {
    public:
        static void writeText(const String& text);
        static void readText(String& text);

        static void writeBinary(const void* data, size_t size);
        static void readBinary(void* data, size_t size);
    };
}

#endif//_EOKAS_NATIVE_PASTEBOARD_H_