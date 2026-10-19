#include "Unit.h"

using namespace eokas;

EOKAS_TEST_CASE(dll) {
    EOKAS_EXPECT(String(Dll::extension) == ".dll");
    Dll missing("eokas-unit-missing.dll");
    EOKAS_EXPECT(!missing.open());
    EOKAS_EXPECT(!missing.isOpen());
    EOKAS_EXPECT(missing.name() == "eokas-unit-missing.dll");
    EOKAS_EXPECT(missing.getSymbol("nope") == nullptr);
    return 0;
}
