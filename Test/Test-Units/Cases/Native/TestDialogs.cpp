#include "Unit.h"
#include "Native/NativeDialogs.h"

using namespace eokas;

EOKAS_TEST_CASE(dialogs) {
    auto alert = &NativeDialogs::alert;
    auto confirm = &NativeDialogs::confirm;
    auto openFile = &NativeDialogs::openFileDialog;
    auto openFolder = &NativeDialogs::openFolderDialog;
    EOKAS_EXPECT(alert != nullptr);
    EOKAS_EXPECT(confirm != nullptr);
    EOKAS_EXPECT(openFile != nullptr);
    EOKAS_EXPECT(openFolder != nullptr);
    return 0;
}
