#include <sdkddkver.h>

#define WIN32_LEAN_AND_MEAN

#include "./Graphics.h"
#include <exception>

const wchar_t* windowTitle = L"test-ui";
const wchar_t* windowClass = L"test-ui";
const int windowWidth = 1000;
const int windowHeight = 720;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    eokas::ui::Graphics* graphics = reinterpret_cast<eokas::ui::Graphics*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
    if (graphics)
    {
        if (graphics->isClosingFloatWindow(hWnd))
        {
            if (message == WM_CLOSE) return 0;
            return DefWindowProcW(hWnd, message, wParam, lParam);
        }
        if (eokas::UICanvas* floating = graphics->floatingCanvas(hWnd))
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
                graphics->canvas().onMouseWheel((float)pt.x, (float)pt.y, step, 0.0f);
            }
            else
            {
                graphics->canvas().onMouseWheel((float)pt.x, (float)pt.y, 0.0f, step);
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
            graphics->canvas().onMouseMove(x, y);
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
            graphics->canvas().onMouseDown(x, y, 0);
        }
        return 0;
    }
    case WM_LBUTTONUP:
    {
        if (graphics)
        {
            float x = (float)(short)LOWORD(lParam);
            float y = (float)(short)HIWORD(lParam);
            graphics->canvas().onMouseUp(x, y, 0);
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
            graphics->canvas().onMouseDown(x, y, 1);
        }
        return 0;
    }
    case WM_RBUTTONUP:
    {
        if (graphics)
        {
            float x = (float)(short)LOWORD(lParam);
            float y = (float)(short)HIWORD(lParam);
            graphics->canvas().onMouseUp(x, y, 1);
        }
        return 0;
    }
    case WM_KEYDOWN:
    {
        if (graphics)
        {
            bool mapped = true;
            eokas::UIKey key = eokas::UIKey::Enter;
            switch (wParam)
            {
            case VK_BACK: key = eokas::UIKey::Backspace; break;
            case VK_DELETE: key = eokas::UIKey::Delete; break;
            case VK_LEFT: key = eokas::UIKey::Left; break;
            case VK_RIGHT: key = eokas::UIKey::Right; break;
            case VK_UP: key = eokas::UIKey::Up; break;
            case VK_DOWN: key = eokas::UIKey::Down; break;
            case VK_HOME: key = eokas::UIKey::Home; break;
            case VK_END: key = eokas::UIKey::End; break;
            case VK_RETURN: key = eokas::UIKey::Enter; break;
            case VK_ESCAPE: key = eokas::UIKey::Escape; break;
            case 'A': key = eokas::UIKey::A; break;
            case 'C': key = eokas::UIKey::C; break;
            case 'X': key = eokas::UIKey::X; break;
            case 'V': key = eokas::UIKey::V; break;
            default: mapped = false; break;
            }
            if (mapped)
            {
                eokas::UIKeyMods mods;
                mods.ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
                mods.shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
                bool textKey = key == eokas::UIKey::A || key == eokas::UIKey::C || key == eokas::UIKey::X || key == eokas::UIKey::V;
                if (!textKey || mods.ctrl)
                {
                    graphics->canvas().onKeyDown(key, mods);
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
                graphics->canvas().onChar(codepoint);
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

int WinMain( _In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow )
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

#if _WIN32
    ::ShowWindow(::GetConsoleWindow(), SW_HIDE);
#endif

    try {
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
        wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
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
        ShowWindow(hWnd, nCmdShow);
        UpdateWindow(hWnd);

        eokas::ui::Graphics graphics;
        graphics.init(hWnd, clientWidth, clientHeight);
        SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)&graphics);

        LARGE_INTEGER freq, last, now;
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&last);

        MSG msg;
        msg.message = static_cast<UINT>(~WM_QUIT);
        while (msg.message != WM_QUIT)
        {
            if (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
            else
            {
                QueryPerformanceCounter(&now);
                float delta = (float)(now.QuadPart - last.QuadPart) / (float)freq.QuadPart;
                last = now;
                graphics.tick(delta);
            }
        }

        graphics.quit();

        return (int)msg.wParam;
    }
    catch (std::exception& e) {
        MessageBoxA(NULL, e.what(), NULL, 0);
        return 1;
    }
}
