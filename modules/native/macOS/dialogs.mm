#include "../dialogs.h"

#include <iostream>
#include <string>
#include <CoreFoundation/CoreFoundation.h>
#include <Cocoa/Cocoa.h>

namespace eokas
{
    void Dialogs::alert(const String& title, const String& message)
    {
        @autoreleasepool {
            NSString* titleStr = [NSString stringWithUTF8String: title.cstr()];
            NSString* messageStr = [NSString stringWithUTF8String: message.cstr()];

            NSAlert *alert = [[NSAlert alloc] init];
            [alert setMessageText: titleStr];
            [alert setInformativeText: messageStr];
            [alert addButtonWithTitle:@"Ok"];
            [alert runModal];
        }
    }

    int Dialogs::confirm(const String& title, const String& message)
    {
        @autoreleasepool {
            NSString* titleStr = [NSString stringWithUTF8String: title.cstr()];
            NSString* messageStr = [NSString stringWithUTF8String: message.cstr()];

            NSAlert *alert = [[NSAlert alloc] init];
            [alert setMessageText: titleStr];
            [alert setInformativeText: messageStr];
            [alert setAlertStyle:NSAlertStyleWarning];
            [alert addButtonWithTitle:@"Yes"];
            [alert addButtonWithTitle:@"No"];
            [alert addButtonWithTitle:@"Cancel"];

            NSInteger ret = [alert runModal];
            switch (ret)
            {
                case NSAlertFirstButtonReturn: return 1;
                case NSAlertSecondButtonReturn: return -1;
                case NSAlertThirdButtonReturn: return 0;
                default: return 0;
            }
        }
    }

    bool Dialogs::openFileDialog(String& selectedPath, const std::map<String, String>& filters)
    {
        @autoreleasepool {
            NSOpenPanel *panel = [NSOpenPanel openPanel];
            panel.message = @"Open";
            panel.prompt = @"Ok";
            panel.canChooseFiles = YES;
            panel.allowsMultipleSelection = NO;

            panel.allowedFileTypes = @[];
            NSMutableArray<NSString *> *allowedTypes = [NSMutableArray array];
            for (auto const& filter : filters) {
                NSString* fileTitle = [NSString stringWithUTF8String: filter.first.cstr()];
                NSString* fileFilter = [NSString stringWithUTF8String: filter.second.cstr()];
                NSString *extension = [fileFilter substringFromIndex:2];
                [allowedTypes addObject:extension];
            }
            panel.allowedFileTypes = [allowedTypes copy];

            if ([panel runModal] == NSModalResponseOK) {
                NSURL *fileURL = [[panel URLs] objectAtIndex:0];
                selectedPath = [[fileURL path] UTF8String];
                return true;
            }
        }
        return false;
    }

    bool Dialogs::openFolderDialog(String& selectedPath, const String& defaultPath)
    {
        @autoreleasepool {
            NSOpenPanel *panel = [NSOpenPanel openPanel];
            panel.message = @"Open";
            panel.prompt = @"Ok";
            panel.canChooseFiles = NO;
            panel.canChooseDirectories = YES;
            panel.allowsMultipleSelection = NO;
            panel.directoryURL = [NSURL URLWithString: [NSString stringWithUTF8String: defaultPath.cstr()]];
            
            if ([panel runModal] == NSModalResponseOK) {
                NSURL *fileURL = [[panel URLs] objectAtIndex:0];
                selectedPath = [[fileURL path] UTF8String];
                return true;
            }
        }
        return false;
    }
}
