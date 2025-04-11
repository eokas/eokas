#ifndef _EOKAS_NATIVE_DIALOGS_H_
#define _EOKAS_NATIVE_DIALOGS_H_

#include "./header.h"

namespace eokas
{
    struct Dialogs
    {
        /**
        * Show an alert box
        * @param title MessageBox title text
        * @param message Message content text
        */
        static void alert(const String& title, const String& message);

        /**
         *
         * @param title MessageBox title text
         * @param message Message content text
         * @return 1: yes, -1: no, 0: cancel
         */
        static int confirm(const String& title, const String& message);

        /**
         * Show a dialog to open file.\n
         * String selectedPath;\n
         * std::map<String, String> filters;\n
         * filters["txt"] = "*.txt";\n
         * OpenFileDialog(selectedPath, filters);
         * @param selectedPath The out selected file path.
         * @param filters File type filters
         * @return True: selected, False: not selected.
         */
        static bool openFileDialog(String& selectedPath, const std::map<String, String>& filters);

        /**
         * Show a dialog to open folder.\n
         * String selectedPath;\n
         * OpenFolderDialog(selectedPath, "");
         * @param selectedPath The out selected folder path.
         * @param defaultPath The default path of the dialog opened.
         * @return True: selected, False: not selected.
         */
        static bool openFolderDialog(String& selectedPath, const String& defaultPath);
    };
}

#endif//_EOKAS_NATIVE_DIALOGS_H_
