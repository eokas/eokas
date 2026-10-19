#include "Unit.h"

using namespace eokas;

EOKAS_TEST_CASE(ascil) {
    Ascil digit('4');
    EOKAS_EXPECT(digit.isNumber());
    EOKAS_EXPECT(digit.isAlphaNumber());
    EOKAS_EXPECT(digit.isAlphaNumber_());
    EOKAS_EXPECT(!digit.isAlpha());
    EOKAS_EXPECT(!digit.isAlpha_());

    EOKAS_EXPECT(Ascil('a').isHex());
    EOKAS_EXPECT(Ascil('F').isHex());
    EOKAS_EXPECT(!Ascil('k').isHex());
    EOKAS_EXPECT(Ascil(' ').isSpace());
    EOKAS_EXPECT(!Ascil('\n').isSpace());
    EOKAS_EXPECT(Ascil('\n').isControl());
    EOKAS_EXPECT(Ascil('b').inRange('a', 'z'));
    EOKAS_EXPECT(!Ascil('b').inRange('A', 'Z'));
    EOKAS_EXPECT(Ascil(',').isPunct());
    EOKAS_EXPECT(!Ascil('y').isPunct());
    EOKAS_EXPECT(Ascil('_').isAlpha_());
    EOKAS_EXPECT(Ascil('_').isAlphaNumber_());
    return 0;
}
