#ifndef _EOKAS_NATIVE_CLIPBOARD_H_
#define _EOKAS_NATIVE_CLIPBOARD_H_

#include "./Header.h"

namespace eokas
{
    class NativeClipboard
    {
    public:
        // Writes UTF-8 text. An empty string clears the clipboard and returns true.
        static bool writeText(const String& text);

        // Reads UTF-8 text. Returns false and clears `text` when the clipboard has no text.
        static bool readText(String& text);

        // Writes bytes in a private clipboard format.
        // A null data pointer or a zero size clears the clipboard and returns true.
        static bool writeBinary(const void* data, size_t size);

        // Copies the private binary payload into `data` when `capacity` is large enough.
        // When a payload exists, `size` is its byte count, including when `capacity` is too small.
        // Returns false when the format is missing, `data` is null, or `capacity` is too small.
        static bool readBinary(void* data, size_t capacity, size_t& size);
    };
}

#endif//_EOKAS_NATIVE_CLIPBOARD_H_
