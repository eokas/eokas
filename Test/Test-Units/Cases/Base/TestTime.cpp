#include "Unit.h"

using namespace eokas;

EOKAS_TEST_CASE(time) {
    TimeSpan span(1, 2, 3, 4);
    EOKAS_EXPECT(span.dayPart() == 1);
    EOKAS_EXPECT(span.hourPart() == 2);
    EOKAS_EXPECT(span.minutePart() == 3);
    EOKAS_EXPECT(span.secondPart() == 4);
    EOKAS_EXPECT(span == TimeSpan(1, 2, 3, 4));

    TimePoint p0(2020, 9, 1, 12, 1, 30);
    TimeSpan s0(0, 0, 20, 0);
    EOKAS_EXPECT(p0.year() == 2020);
    EOKAS_EXPECT(p0.month() == 9);
    EOKAS_EXPECT(p0.date() == 1);
    EOKAS_EXPECT(p0.hour() == 12);
    EOKAS_EXPECT(p0.minute() == 1);
    EOKAS_EXPECT(p0.second() == 30);
    EOKAS_EXPECT((p0 + s0).toString() == "2020-09-01 12:21:30");
    EOKAS_EXPECT((p0 - s0).toString() == "2020-09-01 11:41:30");
    EOKAS_EXPECT((p0 + s0) - p0 == s0);

    return 0;
}
