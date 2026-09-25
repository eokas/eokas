#include "Unit.h"
#include "native/dialogs.h"

using namespace eokas;

EOKAS_TEST_CASE(dialogs) {
    auto alert = &Dialogs::alert;
    auto confirm = &Dialogs::confirm;
    auto openFile = &Dialogs::openFileDialog;
    auto openFolder = &Dialogs::openFolderDialog;
    EOKAS_EXPECT(alert != nullptr);
    EOKAS_EXPECT(confirm != nullptr);
    EOKAS_EXPECT(openFile != nullptr);
    EOKAS_EXPECT(openFolder != nullptr);
    return 0;
}
