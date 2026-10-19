#include <sdkddkver.h>

#define WIN32_LEAN_AND_MEAN

#include "./Graphics.h"
#include <dbghelp.h>
#include <dwmapi.h>
#include <cstdio>
#include <cstring>
#include <exception>
#include <string>

#pragma comment(lib, "dbghelp.lib")
#pragma comment(lib, "dwmapi.lib")

const wchar_t* windowTitle = L"test-ui";
const wchar_t* windowClass = L"test-ui";
const int windowWidth = 1000;
const int windowHeight = 720;

static bool mapUIKey(WPARAM wParam, LPARAM lParam, eokas::UIKey& key)
{
    switch (wParam)
    {
    case VK_BACK: key = eokas::UIKey::Backspace; return true;
    case VK_TAB: key = eokas::UIKey::Tab; return true;
    case VK_RETURN: key = (lParam & (1 << 24)) ? eokas::UIKey::KeypadEnter : eokas::UIKey::Enter; return true;
    case VK_ESCAPE: key = eokas::UIKey::Escape; return true;
    case VK_SPACE: key = eokas::UIKey::Space; return true;
    case VK_INSERT: key = eokas::UIKey::Insert; return true;
    case VK_DELETE: key = eokas::UIKey::Delete; return true;
    case VK_HOME: key = eokas::UIKey::Home; return true;
    case VK_END: key = eokas::UIKey::End; return true;
    case VK_PRIOR: key = eokas::UIKey::PageUp; return true;
    case VK_NEXT: key = eokas::UIKey::PageDown; return true;
    case VK_LEFT: key = eokas::UIKey::Left; return true;
    case VK_RIGHT: key = eokas::UIKey::Right; return true;
    case VK_UP: key = eokas::UIKey::Up; return true;
    case VK_DOWN: key = eokas::UIKey::Down; return true;
    case 'A': key = eokas::UIKey::A; return true;
    case 'B': key = eokas::UIKey::B; return true;
    case 'C': key = eokas::UIKey::C; return true;
    case 'D': key = eokas::UIKey::D; return true;
    case 'E': key = eokas::UIKey::E; return true;
    case 'F': key = eokas::UIKey::F; return true;
    case 'G': key = eokas::UIKey::G; return true;
    case 'H': key = eokas::UIKey::H; return true;
    case 'I': key = eokas::UIKey::I; return true;
    case 'J': key = eokas::UIKey::J; return true;
    case 'K': key = eokas::UIKey::K; return true;
    case 'L': key = eokas::UIKey::L; return true;
    case 'M': key = eokas::UIKey::M; return true;
    case 'N': key = eokas::UIKey::N; return true;
    case 'O': key = eokas::UIKey::O; return true;
    case 'P': key = eokas::UIKey::P; return true;
    case 'Q': key = eokas::UIKey::Q; return true;
    case 'R': key = eokas::UIKey::R; return true;
    case 'S': key = eokas::UIKey::S; return true;
    case 'T': key = eokas::UIKey::T; return true;
    case 'U': key = eokas::UIKey::U; return true;
    case 'V': key = eokas::UIKey::V; return true;
    case 'W': key = eokas::UIKey::W; return true;
    case 'X': key = eokas::UIKey::X; return true;
    case 'Y': key = eokas::UIKey::Y; return true;
    case 'Z': key = eokas::UIKey::Z; return true;
    case '0': key = eokas::UIKey::Digit0; return true;
    case '1': key = eokas::UIKey::Digit1; return true;
    case '2': key = eokas::UIKey::Digit2; return true;
    case '3': key = eokas::UIKey::Digit3; return true;
    case '4': key = eokas::UIKey::Digit4; return true;
    case '5': key = eokas::UIKey::Digit5; return true;
    case '6': key = eokas::UIKey::Digit6; return true;
    case '7': key = eokas::UIKey::Digit7; return true;
    case '8': key = eokas::UIKey::Digit8; return true;
    case '9': key = eokas::UIKey::Digit9; return true;
    case VK_F1: key = eokas::UIKey::F1; return true;
    case VK_F2: key = eokas::UIKey::F2; return true;
    case VK_F3: key = eokas::UIKey::F3; return true;
    case VK_F4: key = eokas::UIKey::F4; return true;
    case VK_F5: key = eokas::UIKey::F5; return true;
    case VK_F6: key = eokas::UIKey::F6; return true;
    case VK_F7: key = eokas::UIKey::F7; return true;
    case VK_F8: key = eokas::UIKey::F8; return true;
    case VK_F9: key = eokas::UIKey::F9; return true;
    case VK_F10: key = eokas::UIKey::F10; return true;
    case VK_F11: key = eokas::UIKey::F11; return true;
    case VK_F12: key = eokas::UIKey::F12; return true;
    case VK_OEM_7: key = eokas::UIKey::Apostrophe; return true;
    case VK_OEM_COMMA: key = eokas::UIKey::Comma; return true;
    case VK_OEM_MINUS: key = eokas::UIKey::Minus; return true;
    case VK_OEM_PERIOD: key = eokas::UIKey::Period; return true;
    case VK_OEM_2: key = eokas::UIKey::Slash; return true;
    case VK_OEM_1: key = eokas::UIKey::Semicolon; return true;
    case VK_OEM_PLUS: key = eokas::UIKey::Equal; return true;
    case VK_OEM_4: key = eokas::UIKey::LeftBracket; return true;
    case VK_OEM_5: key = eokas::UIKey::Backslash; return true;
    case VK_OEM_6: key = eokas::UIKey::RightBracket; return true;
    case VK_OEM_3: key = eokas::UIKey::GraveAccent; return true;
    case VK_CAPITAL: key = eokas::UIKey::CapsLock; return true;
    case VK_SCROLL: key = eokas::UIKey::ScrollLock; return true;
    case VK_NUMLOCK: key = eokas::UIKey::NumLock; return true;
    case VK_SNAPSHOT: key = eokas::UIKey::PrintScreen; return true;
    case VK_PAUSE: key = eokas::UIKey::Pause; return true;
    case VK_NUMPAD0: key = eokas::UIKey::Keypad0; return true;
    case VK_NUMPAD1: key = eokas::UIKey::Keypad1; return true;
    case VK_NUMPAD2: key = eokas::UIKey::Keypad2; return true;
    case VK_NUMPAD3: key = eokas::UIKey::Keypad3; return true;
    case VK_NUMPAD4: key = eokas::UIKey::Keypad4; return true;
    case VK_NUMPAD5: key = eokas::UIKey::Keypad5; return true;
    case VK_NUMPAD6: key = eokas::UIKey::Keypad6; return true;
    case VK_NUMPAD7: key = eokas::UIKey::Keypad7; return true;
    case VK_NUMPAD8: key = eokas::UIKey::Keypad8; return true;
    case VK_NUMPAD9: key = eokas::UIKey::Keypad9; return true;
    case VK_DECIMAL: key = eokas::UIKey::KeypadDecimal; return true;
    case VK_DIVIDE: key = eokas::UIKey::KeypadDivide; return true;
    case VK_MULTIPLY: key = eokas::UIKey::KeypadMultiply; return true;
    case VK_SUBTRACT: key = eokas::UIKey::KeypadSubtract; return true;
    case VK_ADD: key = eokas::UIKey::KeypadAdd; return true;
    case VK_OEM_NEC_EQUAL: key = eokas::UIKey::KeypadEqual; return true;
    case VK_APPS: key = eokas::UIKey::Menu; return true;
    default: return false;
    }
}

static bool characterKey(eokas::UIKey key)
{
    int value = static_cast<int>(key);
    auto between = [value](eokas::UIKey begin, eokas::UIKey end)
    {
        return value >= static_cast<int>(begin) && value <= static_cast<int>(end);
    };
    if (between(eokas::UIKey::A, eokas::UIKey::Z)) return true;
    if (between(eokas::UIKey::Digit0, eokas::UIKey::Digit9)) return true;
    if (key == eokas::UIKey::Space) return true;
    if (between(eokas::UIKey::Apostrophe, eokas::UIKey::GraveAccent)) return true;
    if (between(eokas::UIKey::Keypad0, eokas::UIKey::Keypad9)) return true;
    return key == eokas::UIKey::KeypadDecimal
        || key == eokas::UIKey::KeypadDivide
        || key == eokas::UIKey::KeypadMultiply
        || key == eokas::UIKey::KeypadSubtract
        || key == eokas::UIKey::KeypadAdd
        || key == eokas::UIKey::KeypadEqual;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_ERASEBKGND)
    {
        return 1;
    }
    if (message == WM_PAINT)
    {
        PAINTSTRUCT ps;
        BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
        return 0;
    }

    eokas::ui::Graphics* graphics = reinterpret_cast<eokas::ui::Graphics*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
    if (graphics)
    {
        if (graphics->isClosingFloatWindow(hWnd))
        {
            if (message == WM_CLOSE) return 0;
            return DefWindowProcW(hWnd, message, wParam, lParam);
        }
        if (eokas::UIWindow* floating = graphics->floatingWindow(hWnd))
        {
            if (message == WM_CLOSE) return 0;
            if (message == WM_MOUSEMOVE)
            {
                floating->onMouseMove((float)(short)LOWORD(lParam), (float)(short)HIWORD(lParam));
                return 0;
            }
            if (message == WM_LBUTTONDOWN)
            {
                SetCapture(hWnd);
                floating->onMouseDown((float)(short)LOWORD(lParam), (float)(short)HIWORD(lParam), 0);
                return 0;
            }
            if (message == WM_LBUTTONUP)
            {
                floating->onMouseUp((float)(short)LOWORD(lParam), (float)(short)HIWORD(lParam), 0);
                if (IsWindow(hWnd) && GetCapture() == hWnd) ReleaseCapture();
                return 0;
            }
        }
    }

    switch (message)
    {
    case WM_CLOSE:
    {
        PostQuitMessage(0);
        return 0;
    }
    case WM_MOUSEWHEEL:
    {
        if (graphics)
        {
            POINT pt;
            pt.x = (short)LOWORD(lParam);
            pt.y = (short)HIWORD(lParam);
            ScreenToClient(hWnd, &pt);
            float notches = (float)(short)HIWORD(wParam) / 120.0f;
            float step = -notches * 48.0f;
            bool shift = (LOWORD(wParam) & MK_SHIFT) != 0;
            if (shift)
            {
                graphics->window().onMouseWheel((float)pt.x, (float)pt.y, step, 0.0f);
            }
            else
            {
                graphics->window().onMouseWheel((float)pt.x, (float)pt.y, 0.0f, step);
            }
        }
        return 0;
    }
    case WM_SETCURSOR:
    {
        if (graphics && LOWORD(lParam) == HTCLIENT)
        {
            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(hWnd, &pt);
            bool vertical = false;
            if (graphics->splitterCursor(hWnd, (float)pt.x, (float)pt.y, vertical))
            {
                SetCursor(LoadCursor(nullptr, vertical ? IDC_SIZENS : IDC_SIZEWE));
                return TRUE;
            }
        }
        return DefWindowProcW(hWnd, message, wParam, lParam);
    }
    case WM_MOUSEMOVE:
    {
        if (graphics)
        {
            float x = (float)(short)LOWORD(lParam);
            float y = (float)(short)HIWORD(lParam);
            graphics->hoverDock(hWnd, x, y);
            graphics->window().onMouseMove(x, y);
        }
        return 0;
    }
    case WM_LBUTTONDOWN:
    {
        if (graphics)
        {
            SetCapture(hWnd);
            float x = (float)(short)LOWORD(lParam);
            float y = (float)(short)HIWORD(lParam);
            graphics->window().onMouseDown(x, y, 0);
        }
        return 0;
    }
    case WM_LBUTTONUP:
    {
        if (graphics)
        {
            float x = (float)(short)LOWORD(lParam);
            float y = (float)(short)HIWORD(lParam);
            graphics->window().onMouseUp(x, y, 0);
            ReleaseCapture();
        }
        return 0;
    }
    case WM_RBUTTONDOWN:
    {
        if (graphics)
        {
            float x = (float)(short)LOWORD(lParam);
            float y = (float)(short)HIWORD(lParam);
            graphics->window().onMouseDown(x, y, 1);
        }
        return 0;
    }
    case WM_RBUTTONUP:
    {
        if (graphics)
        {
            float x = (float)(short)LOWORD(lParam);
            float y = (float)(short)HIWORD(lParam);
            graphics->window().onMouseUp(x, y, 1);
        }
        return 0;
    }
    case WM_KEYDOWN:
    {
        if (graphics)
        {
            eokas::UIKey key = eokas::UIKey::Enter;
            if (mapUIKey(wParam, lParam, key))
            {
                eokas::UIKeyMods mods;
                mods.ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
                mods.shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
                mods.alt = (GetKeyState(VK_MENU) & 0x8000) != 0;
                if (!characterKey(key) || mods.ctrl || mods.alt)
                {
                    graphics->window().onKeyDown(key, mods);
                    return 0;
                }
            }
        }
        return DefWindowProcW(hWnd, message, wParam, lParam);
    }
    case WM_CHAR:
    {
        if (graphics)
        {
            static wchar_t pendingHigh = 0;
            wchar_t unit = (wchar_t)wParam;
            uint32_t codepoint = 0;
            if (unit >= 0xD800 && unit <= 0xDBFF)
            {
                pendingHigh = unit;
                return 0;
            }
            if (unit >= 0xDC00 && unit <= 0xDFFF && pendingHigh != 0)
            {
                codepoint = 0x10000 + (((uint32_t)pendingHigh - 0xD800) << 10) + ((uint32_t)unit - 0xDC00);
                pendingHigh = 0;
            }
            else
            {
                pendingHigh = 0;
                codepoint = (uint32_t)unit;
            }

            if (codepoint >= 32 && codepoint != 127)
            {
                graphics->window().onChar(codepoint);
                return 0;
            }
        }
        return DefWindowProcW(hWnd, message, wParam, lParam);
    }
    default:
        return DefWindowProcW(hWnd, message, wParam, lParam);
    }
    return 0;
}

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
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

#if _WIN32
    ::ShowWindow(::GetConsoleWindow(), SW_HIDE);
#endif

    try {
        [[maybe_unused]] VectoredExceptionGuard exceptionStack;
        SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        UINT dpi = GetDpiForSystem();
        if (dpi == 0)
        {
            dpi = 96;
        }
        const int clientWidth = MulDiv(windowWidth, (int)dpi, 96);
        const int clientHeight = MulDiv(windowHeight, (int)dpi, 96);

        WNDCLASSEXW wcex;
        wcex.cbSize = sizeof(WNDCLASSEXW);
        wcex.style = CS_GLOBALCLASS;
        wcex.lpfnWndProc = WndProc;
        wcex.cbClsExtra = 0;
        wcex.cbWndExtra = 0;
        wcex.hInstance = hInstance;
        wcex.hIcon = NULL;
        wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wcex.hbrBackground = nullptr;
        wcex.lpszMenuName = NULL;
        wcex.lpszClassName = windowClass;
        wcex.hIconSm = NULL;
        RegisterClassExW(&wcex);

        RECT windowRect = { 0, 0, clientWidth, clientHeight };
        AdjustWindowRectExForDpi(&windowRect, WS_OVERLAPPEDWINDOW, FALSE, 0, dpi);
        HWND hWnd = CreateWindowW(windowClass, windowTitle, WS_OVERLAPPEDWINDOW,
            80, 40, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top,
            nullptr, nullptr, hInstance, nullptr);
        if (!hWnd)
        {
            return FALSE;
        }
        BOOL useDarkCaption = TRUE;
        DwmSetWindowAttribute(hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDarkCaption, sizeof(useDarkCaption));
        COLORREF captionColor = RGB(0, 0, 0);
        DwmSetWindowAttribute(hWnd, DWMWA_CAPTION_COLOR, &captionColor, sizeof(captionColor));
        COLORREF captionText = RGB(255, 255, 255);
        DwmSetWindowAttribute(hWnd, DWMWA_TEXT_COLOR, &captionText, sizeof(captionText));
        ShowWindow(hWnd, nCmdShow);
        UpdateWindow(hWnd);

        eokas::ui::Graphics graphics;
        graphics.init(hWnd, clientWidth, clientHeight);
        SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)&graphics);

        eokas::Timer timer;

        MSG msg = {};
        while (msg.message != WM_QUIT)
        {
            while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                if (msg.message == WM_QUIT)
                {
                    break;
                }
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
            if (msg.message == WM_QUIT)
            {
                break;
            }
            float delta = (float)eokas::TimeSpan(timer.elapse()).exactSeconds();
            graphics.tick(delta);
        }

        graphics.quit();

        return (int)msg.wParam;
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
