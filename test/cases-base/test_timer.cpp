
#include "../engine/main.h"
using namespace eokas;

_eokas_test_case(timer)
{
    Timer timer;

    timer.reset();
    for(int i = 0; i < 10; i++) {
        i64_t e = timer.elapse();
        printf("elapse: %llu \n", e);
    }
    timer.reset();

    return 0;
}