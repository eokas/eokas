#ifndef _EOKAS_NATIVE_DIALOGS_H_
#define _EOKAS_NATIVE_DIALOGS_H_

#include "./Header.h"

namespace eokas
{
    struct NativeDialogs
    {
        /**
        * Show an alert box
        * @param title MessageBox title text
        * @param message Message content text
        * @param owner Platform window. HWND on Windows, NSWindow* on macOS, X11 Window on Linux. Null uses no owner.
        */
        static void alert(const String& title, const String& message, void* owner = nullptr);

        /**
         *
         * @param title MessageBox title text
         * @param message Message content text
         * @param owner Platform window. HWND on Windows, NSWindow* on macOS, X11 Window on Linux. Null uses no owner.
         * @return 1: yes, -1: no, 0: cancel
         */
        static int confirm(const String& title, const String& message, void* owner = nullptr);

        /**
         * Show a dialog to open file.\n
         * String selectedPath;\n
         * std::map<String, String> filters;\n
         * filters["txt"] = "*.txt";\n
         * OpenFileDialog(selectedPath, filters);
         * @param selectedPath The out selected file path.
         * @param filters File type filters
         * @param owner Platform window. HWND on Windows, NSWindow* on macOS, X11 Window on Linux. Null uses no owner.
         * @return True: selected, False: not selected.
         */
        static bool openFileDialog(String& selectedPath, const std::map<String, String>& filters, void* owner = nullptr);

        /**
         * Show a dialog to open folder.\n
         * String selectedPath;\n
         * OpenFolderDialog(selectedPath, "");
         * @param selectedPath The out selected folder path.
         * @param defaultPath The default path of the dialog opened.
         * @param owner Platform window. HWND on Windows, NSWindow* on macOS, X11 Window on Linux. Null uses no owner.
         * @return True: selected, False: not selected.
         */
        static bool openFolderDialog(String& selectedPath, const String& defaultPath, void* owner = nullptr);
    };
}

#endif//_EOKAS_NATIVE_DIALOGS_H_
