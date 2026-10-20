#include "../NativeDialogs.h"

#import <Cocoa/Cocoa.h>

namespace eokas
{
    namespace
    {
        NSInteger runOwned(void* owner,
            NSInteger (^plain)(void),
            void (^beginSheet)(NSWindow* window, void (^done)(NSModalResponse result)))
        {
            NSWindow* window = (__bridge NSWindow*)owner;
            if (window == nil || !window.isVisible) {
                return plain();
            }
            if (NSApp == nil) {
                [NSApplication sharedApplication];
            }
            beginSheet(window, ^(NSModalResponse result) {
                [NSApp stopModalWithCode:result];
            });
            return [NSApp runModal];
        }

        NSString* utf8String(const String& text)
        {
            const char* utf8 = text.cstr();
            if (utf8 == nullptr) {
                return @"";
            }
            NSString* string = [NSString stringWithUTF8String:utf8];
            return string != nil ? string : @"";
        }

        void appendExtensions(NSMutableArray<NSString*>* types, const String& spec)
        {
            StringVector parts = spec.split(";");
            for (const String& part : parts) {
                String token = part.trim();
                if (token.startsWith("*.")) {
                    token = token.substr(2);
                } else if (token.startsWith(".")) {
                    token = token.substr(1);
                }
                if (token.isEmpty()
                    || token.contains("*")
                    || token.contains("/")
                    || token.contains("\\")
                    || token.contains(" ")) {
                    continue;
                }
                NSString* extension = [NSString stringWithUTF8String:token.cstr()];
                if (extension.length > 0) {
                    [types addObject:extension];
                }
            }
        }

        NSArray<NSString*>* fileExtensions(const std::map<String, String>& filters)
        {
            NSMutableArray<NSString*>* types = [NSMutableArray array];
            for (const auto& filter : filters) {
                appendExtensions(types, filter.second);
            }
            return types;
        }
    }

    void NativeDialogs::alert(const String& title, const String& message, void* owner)
    {
        @autoreleasepool {
            NSAlert* alert = [[NSAlert alloc] init];
            alert.messageText = utf8String(title);
            alert.informativeText = utf8String(message);
            [alert addButtonWithTitle:@"Ok"];
            runOwned(owner, ^{
                return [alert runModal];
            }, ^(NSWindow* window, void (^done)(NSModalResponse)) {
                [alert beginSheetModalForWindow:window completionHandler:done];
            });
        }
    }

    int NativeDialogs::confirm(const String& title, const String& message, void* owner)
    {
        @autoreleasepool {
            NSAlert* alert = [[NSAlert alloc] init];
            alert.messageText = utf8String(title);
            alert.informativeText = utf8String(message);
            alert.alertStyle = NSAlertStyleWarning;
            [alert addButtonWithTitle:@"Yes"];
            [alert addButtonWithTitle:@"No"];
            [alert addButtonWithTitle:@"Cancel"];

            NSInteger result = runOwned(owner, ^{
                return [alert runModal];
            }, ^(NSWindow* window, void (^done)(NSModalResponse)) {
                [alert beginSheetModalForWindow:window completionHandler:done];
            });
            switch (result) {
            case NSAlertFirstButtonReturn: return 1;
            case NSAlertSecondButtonReturn: return -1;
            case NSAlertThirdButtonReturn: return 0;
            default: return 0;
            }
        }
    }

    bool NativeDialogs::openFileDialog(String& selectedPath, const std::map<String, String>& filters, void* owner)
    {
        @autoreleasepool {
            NSOpenPanel* panel = [NSOpenPanel openPanel];
            panel.message = @"Open";
            panel.prompt = @"Ok";
            panel.canChooseFiles = YES;
            panel.canChooseDirectories = NO;
            panel.allowsMultipleSelection = NO;

            NSArray<NSString*>* extensions = fileExtensions(filters);
            if (extensions.count > 0) {
                panel.allowedFileTypes = extensions;
            }

            NSInteger result = runOwned(owner, ^{
                return [panel runModal];
            }, ^(NSWindow* window, void (^done)(NSModalResponse)) {
                [panel beginSheetModalForWindow:window completionHandler:done];
            });
            if (result != NSModalResponseOK || panel.URLs.count == 0) {
                return false;
            }
            NSString* path = panel.URLs[0].path;
            if (path == nil) {
                return false;
            }
            selectedPath = String(path.UTF8String);
            return true;
        }
    }

    bool NativeDialogs::openFolderDialog(String& selectedPath, const String& defaultPath, void* owner)
    {
        @autoreleasepool {
            NSOpenPanel* panel = [NSOpenPanel openPanel];
            panel.message = @"Open";
            panel.prompt = @"Ok";
            panel.canChooseFiles = NO;
            panel.canChooseDirectories = YES;
            panel.allowsMultipleSelection = NO;
            if (!defaultPath.isEmpty()) {
                NSString* path = utf8String(defaultPath);
                if (path.length > 0) {
                    panel.directoryURL = [NSURL fileURLWithPath:path];
                }
            }

            NSInteger result = runOwned(owner, ^{
                return [panel runModal];
            }, ^(NSWindow* window, void (^done)(NSModalResponse)) {
                [panel beginSheetModalForWindow:window completionHandler:done];
            });
            if (result != NSModalResponseOK || panel.URLs.count == 0) {
                return false;
            }
            NSString* path = panel.URLs[0].path;
            if (path == nil) {
                return false;
            }
            selectedPath = String(path.UTF8String);
            return true;
        }
    }
}
