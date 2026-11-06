#include "../NativeUtilities.h"

#import <Cocoa/Cocoa.h>
#import <CoreGraphics/CoreGraphics.h>

namespace eokas
{
    Color NativeUtilities::sampleScreenColor()
    {
        @autoreleasepool {
            CGEventRef event = CGEventCreate(nullptr);
            if (event == nullptr) {
                return Color(0.0f, 0.0f, 0.0f, -1.0f);
            }
            CGPoint point = CGEventGetLocation(event);
            CFRelease(event);

            CGRect rect = CGRectMake(point.x, point.y, 1.0, 1.0);
            CGImageRef image = CGWindowListCreateImage(
                rect, kCGWindowListOptionOnScreenOnly, kCGNullWindowID, kCGWindowImageDefault);
            if (image == nullptr) {
                return Color(0.0f, 0.0f, 0.0f, -1.0f);
            }

            CGColorSpaceRef space = CGColorSpaceCreateDeviceRGB();
            unsigned char bytes[4] = {0, 0, 0, 0};
            CGContextRef context = CGBitmapContextCreate(
                bytes, 1, 1, 8, 4, space, kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
            CGColorSpaceRelease(space);
            if (context == nullptr) {
                CGImageRelease(image);
                return Color(0.0f, 0.0f, 0.0f, -1.0f);
            }
            CGContextDrawImage(context, CGRectMake(0, 0, 1, 1), image);
            CGContextRelease(context);
            CGImageRelease(image);

            return Color(
                static_cast<float>(bytes[0]) / 255.0f,
                static_cast<float>(bytes[1]) / 255.0f,
                static_cast<float>(bytes[2]) / 255.0f,
                1.0f);
        }
    }
}
