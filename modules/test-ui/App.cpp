#include <sdkddkver.h>

#define WIN32_LEAN_AND_MEAN

#include "./Graphics.h"
#include <exception>

const char* windowTitle = "test-ui";
const char* windowClass = "test-ui";
const int windowWidth = 800;
const int windowHeight = 600;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    eokas::ui::Graphics* graphics = reinterpret_cast<eokas::ui::Graphics*>(GetWindowLongPtrA(hWnd, GWLP_USERDATA));

    switch (message)
    {
    case WM_CLOSE:
    {
        PostQuitMessage(0);
        return 0;
    }
    case WM_MOUSEMOVE:
    {
        if (graphics)
        {
            float x = (float)(short)LOWORD(lParam);
            float y = (float)(short)HIWORD(lParam);
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
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
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

        WNDCLASSEXA wcex;
        wcex.cbSize = sizeof(WNDCLASSEXA);
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
        RegisterClassExA(&wcex);

        RECT windowRect = { 0, 0, windowWidth, windowHeight };
        AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);
        HWND hWnd = CreateWindowA(windowClass, windowTitle, WS_OVERLAPPEDWINDOW,
            0, 0, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top,
            nullptr, nullptr, hInstance, nullptr);
        if (!hWnd)
        {
            return FALSE;
        }
        ShowWindow(hWnd, nCmdShow);
        UpdateWindow(hWnd);

        eokas::ui::Graphics graphics;
        graphics.init(hWnd, windowWidth, windowHeight);
        SetWindowLongPtrA(hWnd, GWLP_USERDATA, (LONG_PTR)&graphics);

        LARGE_INTEGER freq, last, now;
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&last);

        MSG msg;
        msg.message = static_cast<UINT>(~WM_QUIT);
        while (msg.message != WM_QUIT)
        {
            if (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessageA(&msg);
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
