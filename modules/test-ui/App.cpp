#include <sdkddkver.h>

#define WIN32_LEAN_AND_MEAN

#include "./Graphics.h"

const char* windowTitle = "test-ui";
const char* windowClass = "test-ui";
const int windowWidth = 800;
const int windowHeight = 600;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CLOSE:
    {
        PostQuitMessage(0);
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

        HWND hWnd = CreateWindowA(windowClass, windowTitle, WS_OVERLAPPEDWINDOW,
            0, 0, windowWidth, windowHeight, nullptr, nullptr, hInstance, nullptr);
        if (!hWnd)
        {
            return FALSE;
        }
        ShowWindow(hWnd, nCmdShow);
        UpdateWindow(hWnd);

        eokas::ui::Graphics graphics;
        graphics.init(hWnd, windowWidth, windowHeight);

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
