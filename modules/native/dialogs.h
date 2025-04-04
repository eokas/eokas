#ifndef _EOKAS_NATIVE_DIALOGS_H_
#define _EOKAS_NATIVE_DIALOGS_H_

#include "./header.h"

namespace eokas
{
    struct Dialogs
    {
        /**
            String selectedPath;
            std::map<String, String> filters;
            filters["txt"] = "*.txt";
            OpenFileDialog(selectedPath, filters);
        */
        static bool OpenFileDialog(String& selectedPath, const std::map<String, String>& filters);
        
        /**
            String selectedPath;
            OpenFolderDialog(selectedPath, "");
         */
        static bool OpenFolderDialog(String& selectedPath, const String& defaultPath);
    };
}

#endif//_EOKAS_NATIVE_DIALOGS_H_
