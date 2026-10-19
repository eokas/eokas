#include "Unit.h"

using namespace eokas;

namespace {

class Probe : public Object {
public:
    explicit Probe(int value)
        : value(value) {
    }

    int value;
};

}

EOKAS_TEST_CASE(table) {
    Table<int, Object> table;
    Probe* item = static_cast<Probe*>(table.insert<Probe>(1, 42));
    EOKAS_EXPECT(item != nullptr);
    EOKAS_EXPECT(item->value == 42);
    EOKAS_EXPECT(table.select(1) == item);
    EOKAS_EXPECT(table.insert<Probe>(1, 7) == nullptr);
    EOKAS_EXPECT(table.select(2) == nullptr);

    table.remove(1);
    EOKAS_EXPECT(table.select(1) == nullptr);
    return 0;
}
