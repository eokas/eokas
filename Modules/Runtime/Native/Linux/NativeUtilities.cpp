#include "../NativeUtilities.h"
#include "NativeInternal.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>

namespace eokas
{
    namespace
    {
        float channelOf(unsigned long pixel, unsigned long mask)
        {
            if (mask == 0) {
                return 0.0f;
            }
            int shift = 0;
            unsigned long bit = mask;
            while ((bit & 1UL) == 0) {
                bit >>= 1;
                ++shift;
            }
            int bits = 0;
            while ((bit & 1UL) != 0) {
                bit >>= 1;
                ++bits;
            }
            unsigned long value = (pixel & mask) >> shift;
            float max = bits > 0 ? static_cast<float>((1u << bits) - 1) : 1.0f;
            return static_cast<float>(value) / max;
        }
    }

    Color NativeUtilities::sampleScreenColor()
    {
        ensureXThreads();
        ::Display* display = XOpenDisplay(nullptr);
        if (display == nullptr) {
            return Color(0.0f, 0.0f, 0.0f, -1.0f);
        }

        ::Window root = DefaultRootWindow(display);
        ::Window rootReturn = None;
        ::Window childReturn = None;
        int rootX = 0;
        int rootY = 0;
        int winX = 0;
        int winY = 0;
        unsigned int mask = 0;
        if (XQueryPointer(display, root, &rootReturn, &childReturn, &rootX, &rootY, &winX, &winY, &mask) == False) {
            XCloseDisplay(display);
            return Color(0.0f, 0.0f, 0.0f, -1.0f);
        }

        XImage* image = XGetImage(display, root, rootX, rootY, 1, 1, AllPlanes, ZPixmap);
        if (image == nullptr) {
            XCloseDisplay(display);
            return Color(0.0f, 0.0f, 0.0f, -1.0f);
        }

        unsigned long pixel = XGetPixel(image, 0, 0);
        Color color(
            channelOf(pixel, image->red_mask),
            channelOf(pixel, image->green_mask),
            channelOf(pixel, image->blue_mask),
            1.0f);
        XDestroyImage(image);
        XCloseDisplay(display);
        return color;
    }
}
