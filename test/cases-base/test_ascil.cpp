
#include "../engine/main.h"
using namespace eokas;

_eokas_test_case(ascil)
{
    {
        Ascil c('4');
        _eokas_test_check(c.isNumber() && c.isAlphaNumber() && c.isAlphaNumber_());
        _eokas_test_check(!(c.isAlpha() && c.isAlpha_()));
    }

    {
        Ascil a('a');
        Ascil k('k');
        _eokas_test_check(a.isHex() && !k.isHex());
    }

    {
        Ascil space (' ');
        Ascil enter('\n');
        _eokas_test_check(space.isSpace() && !enter.isSpace());
    }

    {
        Ascil x('b');
        _eokas_test_check(x.inRange('a', 'z') && !x.inRange('A', 'Z'));
    }

    {
        Ascil x(',');
        Ascil y('y');
        _eokas_test_check(x.isPunct() && !y.isPunct());
    }

    return 0;
}