#include "Unit.h"

using namespace eokas;

EOKAS_TEST_CASE(math) {
    Vector2 a(3.0f, 4.0f);
    EOKAS_EXPECT(_FloatEqual(a.sqrmagnitude(), 25.0f));
    EOKAS_EXPECT(_FloatEqual(a.magnitude(), 5.0f));
    EOKAS_EXPECT((Vector2(1.0f, 2.0f) + Vector2(3.0f, 4.0f)) == Vector2(4.0f, 6.0f));
    EOKAS_EXPECT(_FloatEqual(Vector2::dot(Vector2::RIGHT, Vector2::UP), 0.0f));
    EOKAS_EXPECT(Vector3::cross(Vector3::RIGHT, Vector3::UP) == Vector3::FORWARD);

    EOKAS_EXPECT(Math::min_s(3, 7) == 3);
    EOKAS_EXPECT(Math::max_s(3, 7) == 7);
    EOKAS_EXPECT(_FloatEqual(Math::clamp(5.0f, 0.0f, 3.0f), 3.0f));
    EOKAS_EXPECT(_FloatEqual(Math::lerp(0.0f, 10.0f, 0.25f), 2.5f));
    EOKAS_EXPECT(_FloatEqual(Math::radianToAngle(Math::PI), 180.0f));
    EOKAS_EXPECT(_FloatEqual(Math::angleToRadian(180.0f), Math::PI));

    return 0;
}
