#include "../NativeUtilities.h"

#if _WIN32
#include <windows.h>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

namespace eokas
{
    Color NativeUtilities::sampleScreenColor()
    {
        POINT point;
        if (GetCursorPos(&point) == FALSE) {
            return Color(0.0f, 0.0f, 0.0f, -1.0f);
        }

        HDC dc = GetDC(nullptr);
        if (dc == nullptr) {
            return Color(0.0f, 0.0f, 0.0f, -1.0f);
        }

        COLORREF pixel = GetPixel(dc, point.x, point.y);
        ReleaseDC(nullptr, dc);
        if (pixel == CLR_INVALID) {
            return Color(0.0f, 0.0f, 0.0f, -1.0f);
        }

        return Color(
            static_cast<float>(GetRValue(pixel)) / 255.0f,
            static_cast<float>(GetGValue(pixel)) / 255.0f,
            static_cast<float>(GetBValue(pixel)) / 255.0f,
            1.0f);
    }
}

#endif
