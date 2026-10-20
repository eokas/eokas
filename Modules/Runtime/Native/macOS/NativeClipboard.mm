#include "../NativeClipboard.h"

#import <Cocoa/Cocoa.h>

#include <cstring>

namespace eokas
{
    namespace
    {
        NSString* const kBinaryType = @"com.eokas.native.clipboard.binary";

        NSPasteboard* board()
        {
            return [NSPasteboard generalPasteboard];
        }
    }

    bool NativeClipboard::writeText(const String& text)
    {
        @autoreleasepool {
            NSPasteboard* pasteboard = board();
            if (text.isEmpty()) {
                [pasteboard clearContents];
                return true;
            }
            const char* utf8 = text.cstr();
            NSString* string = utf8 ? [NSString stringWithUTF8String:utf8] : nil;
            if (string == nil) {
                return false;
            }
            if (![pasteboard declareTypes:@[NSPasteboardTypeString] owner:nil]) {
                return false;
            }
            return [pasteboard setString:string forType:NSPasteboardTypeString];
        }
    }

    bool NativeClipboard::readText(String& text)
    {
        text.clear();
        @autoreleasepool {
            NSString* string = [board() stringForType:NSPasteboardTypeString];
            if (string == nil) {
                return false;
            }
            const char* utf8 = string.UTF8String;
            if (utf8 != nullptr) {
                text = String(utf8);
            }
            return true;
        }
    }

    bool NativeClipboard::writeBinary(const void* data, size_t size)
    {
        if (data == nullptr && size > 0) {
            return false;
        }
        @autoreleasepool {
            NSPasteboard* pasteboard = board();
            [pasteboard clearContents];
            if (data == nullptr || size == 0) {
                return true;
            }
            NSData* bytes = [NSData dataWithBytes:data length:size];
            if (bytes == nil) {
                return false;
            }
            [pasteboard declareTypes:@[kBinaryType] owner:nil];
            return [pasteboard setData:bytes forType:kBinaryType];
        }
    }

    bool NativeClipboard::readBinary(void* data, size_t capacity, size_t& size)
    {
        size = 0;
        @autoreleasepool {
            NSData* bytes = [board() dataForType:kBinaryType];
            if (bytes == nil) {
                return false;
            }
            size = static_cast<size_t>(bytes.length);
            if (data == nullptr || capacity < size) {
                return false;
            }
            if (size > 0 && bytes.bytes != nullptr) {
                std::memcpy(data, bytes.bytes, size);
            }
            return true;
        }
    }
}
