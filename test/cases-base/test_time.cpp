
#include "../engine/main.h"
using namespace eokas;

_eokas_test_case(time)
{
    TimePoint now;
    printf("now: %s\n", now.toString().cstr());
    printf("now: %llu\n", now.timestamp());

    TimePoint p0(2020, 9, 1, 12, 1, 30);
    TimeSpan s0(0, 0, 20, 0);
    printf("p0: %s\n", p0.toString().cstr());
    printf("s0: %lld days, %lld hours, %lld minutes, %lld seconds\n", s0.dayPart(), s0.hourPart(), s0.minutePart(), s0.secondPart());
    printf("s0: %f\n", s0.exactHours());

    TimePoint p1 = p0 + s0;
    TimeSpan s1 = p1 - p0;
    printf("p1: %s\n", p1.toString().cstr());
    printf("s1: %lld days, %lld hours, %lld minutes, %lld seconds\n", s1.dayPart(), s1.hourPart(), s1.minutePart(), s1.secondPart());
    printf("s1: %f hours\n", s1.exactHours());

    _eokas_test_check((p0 + s0).toString() == "2020-09-01 12:21:30");
    _eokas_test_check((p0 - s0).toString() == "2020-09-01 11:41:30");

    _eokas_test_check(s0 == s1);

    return 0;
}
