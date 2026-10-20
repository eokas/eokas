#include "../NativeClipboard.h"

#if _WIN32
#include <windows.h>

#include <cstdint>
#include <cstring>

#pragma comment(lib, "user32.lib")

namespace eokas
{
    namespace
    {
        constexpr wchar_t kBinaryFormatName[] = L"eokas.NativeClipboard.binary";
        constexpr wchar_t kClipboardClass[] = L"eokas.NativeClipboard";

        UINT binaryFormat()
        {
            static UINT format = RegisterClipboardFormatW(kBinaryFormatName);
            return format;
        }

        HWND clipboardWindow()
        {
            static HWND window = nullptr;
            if (window != nullptr && IsWindow(window)) {
                return window;
            }

            HINSTANCE instance = GetModuleHandleW(nullptr);
            WNDCLASSW windowClass = {};
            windowClass.lpfnWndProc = DefWindowProcW;
            windowClass.hInstance = instance;
            windowClass.lpszClassName = kClipboardClass;
            ATOM atom = RegisterClassW(&windowClass);
            if (atom == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
                return nullptr;
            }
            window = CreateWindowExW(0, kClipboardClass, L"", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, instance, nullptr);
            return window;
        }

        class ClipboardLock
        {
        public:
            explicit ClipboardLock(HWND owner)
                : open(owner != nullptr && OpenClipboard(owner) != FALSE)
            {
            }

            ~ClipboardLock()
            {
                if (open) {
                    CloseClipboard();
                }
            }

            bool open;
        };

        bool setClipboardData(UINT format, const void* bytes, size_t size)
        {
            HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, size);
            if (memory == nullptr) {
                return false;
            }
            void* dest = GlobalLock(memory);
            if (dest == nullptr) {
                GlobalFree(memory);
                return false;
            }
            if (size > 0 && bytes != nullptr) {
                std::memcpy(dest, bytes, size);
            }
            GlobalUnlock(memory);
            if (SetClipboardData(format, memory) == nullptr) {
                GlobalFree(memory);
                return false;
            }
            return true;
        }
    }

    bool NativeClipboard::writeText(const String& text)
    {
        ClipboardLock lock(clipboardWindow());
        if (!lock.open || !EmptyClipboard()) {
            return false;
        }
        if (text.isEmpty()) {
            return true;
        }

        const char* utf8 = text.cstr();
        std::wstring wide = utf8 ? String::utf8ToUnicode(utf8, false) : std::wstring();
        size_t bytes = (wide.size() + 1) * sizeof(wchar_t);
        return setClipboardData(CF_UNICODETEXT, wide.c_str(), bytes);
    }

    bool NativeClipboard::readText(String& text)
    {
        text.clear();
        ClipboardLock lock(clipboardWindow());
        if (!lock.open || !IsClipboardFormatAvailable(CF_UNICODETEXT)) {
            return false;
        }
        HANDLE memory = GetClipboardData(CF_UNICODETEXT);
        if (memory == nullptr) {
            return false;
        }
        const wchar_t* wide = static_cast<const wchar_t*>(GlobalLock(memory));
        if (wide == nullptr) {
            return false;
        }
        text = String::unicodeToUtf8(wide, false);
        GlobalUnlock(memory);
        return true;
    }

    bool NativeClipboard::writeBinary(const void* data, size_t size)
    {
        if ((data == nullptr && size > 0) || size > 0xFFFFFFFFu) {
            return false;
        }
        UINT format = binaryFormat();
        if (format == 0) {
            return false;
        }
        ClipboardLock lock(clipboardWindow());
        if (!lock.open || !EmptyClipboard()) {
            return false;
        }
        if (data == nullptr || size == 0) {
            return true;
        }

        uint32_t payload = static_cast<uint32_t>(size);
        size_t bytes = sizeof(payload) + size;
        HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, bytes);
        if (memory == nullptr) {
            return false;
        }
        auto* dest = static_cast<unsigned char*>(GlobalLock(memory));
        if (dest == nullptr) {
            GlobalFree(memory);
            return false;
        }
        std::memcpy(dest, &payload, sizeof(payload));
        std::memcpy(dest + sizeof(payload), data, size);
        GlobalUnlock(memory);
        if (SetClipboardData(format, memory) == nullptr) {
            GlobalFree(memory);
            return false;
        }
        return true;
    }

    bool NativeClipboard::readBinary(void* data, size_t capacity, size_t& size)
    {
        size = 0;
        UINT format = binaryFormat();
        if (format == 0) {
            return false;
        }
        ClipboardLock lock(clipboardWindow());
        if (!lock.open || !IsClipboardFormatAvailable(format)) {
            return false;
        }
        HANDLE memory = GetClipboardData(format);
        if (memory == nullptr) {
            return false;
        }
        const auto* src = static_cast<const unsigned char*>(GlobalLock(memory));
        if (src == nullptr) {
            return false;
        }
        SIZE_T stored = GlobalSize(memory);
        bool ok = false;
        if (stored >= sizeof(uint32_t)) {
            uint32_t payload = 0;
            std::memcpy(&payload, src, sizeof(payload));
            if (stored >= sizeof(payload) + payload) {
                size = payload;
                if (data != nullptr && capacity >= payload) {
                    if (payload > 0) {
                        std::memcpy(data, src + sizeof(payload), payload);
                    }
                    ok = true;
                }
            }
        }
        GlobalUnlock(memory);
        return ok;
    }
}

#endif
