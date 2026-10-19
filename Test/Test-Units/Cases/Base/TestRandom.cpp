#include "Unit.h"

using namespace eokas;

EOKAS_TEST_CASE(random) {
    f32_t sample = Random::value();
    EOKAS_EXPECT(sample >= 0.0f && sample <= 1.0f);

    for (int i = 0; i < 32; ++i) {
        f32_t value = Random::range(0.0f, 1.0f);
        EOKAS_EXPECT(value >= 0.0f && value <= 1.0f);
        i32_t index = Random::range(0, 10);
        EOKAS_EXPECT(index >= 0 && index <= 10);
        f64_t wide = Random::range(2.0, 4.0);
        EOKAS_EXPECT(wide >= 2.0 && wide <= 4.0);
    }

    Random_Fake seeded(1);
    f32_t first = seeded.make();
    Random_Fake again(1);
    EOKAS_EXPECT(_FloatEqual(first, again.make()));
    return 0;
}
