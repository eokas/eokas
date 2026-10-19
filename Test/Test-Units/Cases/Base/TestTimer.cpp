#include "Unit.h"

using namespace eokas;

EOKAS_TEST_CASE(timer) {
    Timer timer;
    volatile int sink = 0;
    for (int i = 0; i < 1000; ++i) {
        sink += i;
    }
    i64_t elapsed = timer.elapse(false);
    EOKAS_EXPECT(elapsed >= 0);
    timer.reset();
    EOKAS_EXPECT(timer.elapse() >= 0);
    EOKAS_EXPECT(sink > 0);
    return 0;
}
