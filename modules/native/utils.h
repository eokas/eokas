#ifndef _EOKAS_NATIVE_UTILS_H_
#define _EOKAS_NATIVE_UTILS_H_

#include "./header.h"

namespace eokas {
    /**
        String selectedPath;
        std::map<String, String> filters;
        filters["txt"] = "*.txt";
        OpenFileDialog(selectedPath, filters);
     */
    bool OpenFileDialog(String& selectedPath, const std::map<String, String>& filters);
    
    /*
        String selectedPath;
        OpenFolderDialog(selectedPath, "");
     */
    bool OpenFolderDialog(String& selectedPath, const String& defaultPath);
}

#endif//_EOKAS_NATIVE_UTILS_H_
