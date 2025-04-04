
#include "../engine/main.h"
using namespace eokas;

_eokas_test_case(access)
{
    int a = 5;
    AccessRef<int, true, false> pa(a);
    printf("pa: %d\n", pa.get());
    _eokas_test_check(pa.get() == a);

    int b = 6;
    AccessValue<int, true, true> pb;
    pb.set(b);
    printf("pb: %d\n", pb.get());
    _eokas_test_check(pb.get() == b);

    return 0;
}
