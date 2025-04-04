
#include "../engine/main.h"
using namespace eokas;

_eokas_test_case(random)
{
    {
        f32_t x = Random::value();
        printf("x: %f\n", x);
    }

    printf("random-float: ");
    for(int i = 0; i < 100; i++) {
        f32_t x = Random::range(0.0f, 1.0f);
        printf("%f, ", x);
        _eokas_test_check(x >=0 && x < 10);
    }
    printf("\n");

    printf("random-int: ");
    for(int i = 0; i < 100; i++) {
        i32_t x = Random::range(0, 10);
        printf("%d, ", x);
        _eokas_test_check(x >= 0 && x < 10);
    }
    printf("\n");

    return 0;
}
