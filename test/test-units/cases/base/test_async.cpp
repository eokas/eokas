#include "Unit.h"
#include "base/async.h"

using namespace eokas;

EOKAS_TEST_CASE(async) {
    ThreadPool pool(2);
    EOKAS_EXPECT(pool.size() >= 1);
    auto sum = pool.exec([](int a, int b) { return a + b; }, 20, 22);
    EOKAS_EXPECT(sum.get() == 42);
    EOKAS_EXPECT(pool.idle_size() >= 0);
    return 0;
}
