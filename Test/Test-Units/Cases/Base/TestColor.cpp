#include "Unit.h"

using namespace eokas;

EOKAS_TEST_CASE(color) {
    Color c(0.2f, 0.4f, 0.6f, 1.0f);
    Color d(0.2f, 0.1f, 0.1f, 0.0f);
    EOKAS_EXPECT((c + d) == Color(0.4f, 0.5f, 0.7f, 1.0f));
    EOKAS_EXPECT((c - d) == Color(0.0f, 0.3f, 0.5f, 1.0f));
    EOKAS_EXPECT((-Color(0.0f, 0.0f, 0.0f, 0.0f)) == Color(1.0f, 1.0f, 1.0f, 1.0f));

    Color overflow(1.5f, -0.2f, 0.5f, 2.0f);
    overflow.clamp();
    EOKAS_EXPECT(overflow == Color(1.0f, 0.0f, 0.5f, 1.0f));
    EOKAS_EXPECT(Color(0.1f, 0.2f, 0.3f, 0.4f) != Color(0.1f, 0.2f, 0.3f, 0.5f));

    return 0;
}
