#include <sdkddkver.h>

#define WIN32_LEAN_AND_MEAN

#include "./Graphics.h"
#include <dbghelp.h>
#include <cstdio>
#include <cstring>
#include <exception>
#include <string>

#pragma comment(lib, "dbghelp.lib")

const int windowWidth = 1000;
const int windowHeight = 720;

namespace
{
    constexpr DWORD kCppExceptionCode = 0xE06D7363;

    thread_local char g_exceptionStack[12288];
    thread_local bool g_capturingExceptionStack = false;

    BOOL CALLBACK initDebugSymbols(PINIT_ONCE, PVOID, PVOID*)
    {
        SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES | SYMOPT_FAIL_CRITICAL_ERRORS);
        SymInitialize(GetCurrentProcess(), nullptr, TRUE);
        return TRUE;
    }

    bool isExceptionRuntimeFrame(const char* name)
    {
        if (name == nullptr)
        {
            return false;
        }
        const char* skip[] = {
            "captureExceptionStack",
            "vectoredExceptionHandler",
            "_CxxThrowException",
            "RaiseException",
            "KiUserExceptionDispatcher",
            "RtlRaiseException",
            "RtlCaptureStackBackTrace",
        };
        for (const char* item : skip)
        {
            if (strstr(name, item) != nullptr)
            {
                return true;
            }
        }
        return false;
    }

    void appendText(char* dest, size_t destSize, size_t& used, const char* text)
    {
        if (text == nullptr || used + 1 >= destSize)
        {
            return;
        }
        size_t length = strlen(text);
        if (used + length >= destSize)
        {
            length = destSize - used - 1;
        }
        memcpy(dest + used, text, length);
        used += length;
        dest[used] = '\0';
    }

    void captureExceptionStack()
    {
        void* frames[48] = {};
        USHORT count = CaptureStackBackTrace(0, 48, frames, nullptr);
        g_exceptionStack[0] = '\0';
        if (count == 0)
        {
            return;
        }

        static INIT_ONCE symOnce = INIT_ONCE_STATIC_INIT;
        InitOnceExecuteOnce(&symOnce, initDebugSymbols, nullptr, nullptr);

        HANDLE process = GetCurrentProcess();
        alignas(SYMBOL_INFO) char symbolBuffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME] = {};
        auto* symbol = reinterpret_cast<SYMBOL_INFO*>(symbolBuffer);
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = MAX_SYM_NAME;

        size_t used = 0;
        int written = 0;
        for (int pass = 0; pass < 2 && written == 0; ++pass)
        {
            bool skipRuntime = pass == 0;
            used = 0;
            g_exceptionStack[0] = '\0';
            for (USHORT i = 0; i < count; ++i)
            {
                DWORD64 address = reinterpret_cast<DWORD64>(frames[i]);
                char line[2048];
                DWORD64 displacement = 0;
                if (SymFromAddr(process, address, &displacement, symbol))
                {
                    if (skipRuntime && isExceptionRuntimeFrame(symbol->Name))
                    {
                        continue;
                    }
                    IMAGEHLP_LINE64 fileLine = {};
                    fileLine.SizeOfStruct = sizeof(fileLine);
                    DWORD lineDisplacement = 0;
                    if (SymGetLineFromAddr64(process, address, &lineDisplacement, &fileLine) && fileLine.FileName != nullptr)
                    {
                        _snprintf_s(line, _TRUNCATE, "  %s (%s:%lu)\n", symbol->Name, fileLine.FileName, fileLine.LineNumber);
                    }
                    else
                    {
                        _snprintf_s(line, _TRUNCATE, "  %s + 0x%llX\n", symbol->Name, static_cast<unsigned long long>(displacement));
                    }
                }
                else
                {
                    _snprintf_s(line, _TRUNCATE, "  0x%llX\n", static_cast<unsigned long long>(address));
                }
                appendText(g_exceptionStack, sizeof(g_exceptionStack), used, line);
                ++written;
            }
        }
    }

    LONG CALLBACK vectoredExceptionHandler(EXCEPTION_POINTERS* info)
    {
        if (g_capturingExceptionStack || info == nullptr || info->ExceptionRecord == nullptr)
        {
            return EXCEPTION_CONTINUE_SEARCH;
        }
        DWORD code = info->ExceptionRecord->ExceptionCode;
        if (code != kCppExceptionCode)
        {
            return EXCEPTION_CONTINUE_SEARCH;
        }

        g_capturingExceptionStack = true;
        captureExceptionStack();
        g_capturingExceptionStack = false;
        return EXCEPTION_CONTINUE_SEARCH;
    }

    struct VectoredExceptionGuard
    {
        PVOID handle = nullptr;

        VectoredExceptionGuard()
        {
            g_exceptionStack[0] = '\0';
            handle = AddVectoredExceptionHandler(1, vectoredExceptionHandler);
        }

        ~VectoredExceptionGuard()
        {
            if (handle != nullptr)
            {
                RemoveVectoredExceptionHandler(handle);
            }
        }
    };

    void showExceptionMessage(const char* message)
    {
        std::string text = (message != nullptr && message[0] != '\0') ? message : "Unknown exception.";
        if (g_exceptionStack[0] != '\0')
        {
            text += "\n\nStack trace:\n";
            text += g_exceptionStack;
        }
        MessageBoxA(nullptr, text.c_str(), "test-ui", MB_OK | MB_ICONERROR);
    }
}

int WinMain( _In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow )
{
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

#if _WIN32
    ::ShowWindow(::GetConsoleWindow(), SW_HIDE);
#endif

    try {
        [[maybe_unused]] VectoredExceptionGuard exceptionStack;

        eokas::ui::Graphics graphics;
        if (!graphics.init((float)windowWidth, (float)windowHeight))
        {
            return FALSE;
        }

        eokas::Timer timer;
        while (graphics.isOpen())
        {
            graphics.pump();
            if (!graphics.isOpen())
            {
                break;
            }
            float delta = (float)eokas::TimeSpan(timer.elapse()).exactSeconds();
            graphics.tick(delta);
        }

        graphics.quit();
        return 0;
    }
    catch (const std::exception& e)
    {
        showExceptionMessage(e.what());
        return 1;
    }
    catch (...)
    {
        showExceptionMessage("Unknown exception.");
        return 1;
    }
}
