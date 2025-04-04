#include "../dialogs.h"

#include <iostream>
#include <string>
#include <CoreFoundation/CoreFoundation.h>
#include <Cocoa/Cocoa.h>

namespace eokas
{
    bool Dialogs::OpenFileDialog(String& selectedPath, const std::map<String, String>& filters) {
        @autoreleasepool {
            NSOpenPanel *panel = [NSOpenPanel openPanel];
            panel.message = @"Open";
            panel.prompt = @"OK";
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
    
    bool Dialogs::OpenFolderDialog(String& selectedPath, const String& defaultPath) {
        @autoreleasepool {
            NSOpenPanel *panel = [NSOpenPanel openPanel];
            panel.message = @"Open";
            panel.prompt = @"OK";
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
