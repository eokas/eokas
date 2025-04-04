// // 包含 SDKDDKVer.h 可定义可用的最高版本的 Windows 平台。
// 如果希望为之前的 Windows 平台构建应用程序，在包含 SDKDDKVer.h 之前请先包含 WinSDKVer.h 并
// 将 _WIN32_WINNT 宏设置为想要支持的平台。
#include <sdkddkver.h>

// 从 Windows 头文件中排除极少使用的内容
#define WIN32_LEAN_AND_MEAN             

#include "./Graphics.h"

// 全局变量:
const char* windowTitle = "001-HelloWorld";
const char* windowClass = "001-HelloWorld";
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

// int main(int argc, char** argv)
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

        eokas::gpu::Graphics graphics;
        graphics.init(hWnd, windowWidth, windowHeight);

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
                graphics.tick(0);
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

