#include "Unit.h"

using namespace eokas;

EOKAS_TEST_CASE(access) {
    int stored = 5;
    int modified = 0;
    AccessRef<int, true, true> reader(stored);
    reader.onModify([&](const int& value) { modified = value; });
    EOKAS_EXPECT(reader.get() == 5);
    reader.set(9);
    EOKAS_EXPECT(stored == 9);
    EOKAS_EXPECT(modified == 9);

    AccessValue<int> value(6);
    value = 8;
    EOKAS_EXPECT(value.get() == 8);
    EOKAS_EXPECT((int)value == 8);
    return 0;
}
