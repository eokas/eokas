#include "Unit.h"

using namespace eokas;

namespace {

struct Pooled {
    int value;
    explicit Pooled(int value)
        : value(value) {
    }
};

}

EOKAS_TEST_CASE(pool) {
    Pool<Pooled> pool;
    EOKAS_EXPECT(pool.empty());
    Pooled* first = pool.acquire(3);
    EOKAS_EXPECT(first != nullptr);
    EOKAS_EXPECT(first->value == 3);
    EOKAS_EXPECT(pool.size() == 1);

    pool.release(first);
    EOKAS_EXPECT(pool.empty());
    Pooled* second = pool.acquire(8);
    EOKAS_EXPECT(second == first);
    EOKAS_EXPECT(second->value == 8);
    return 0;
}
