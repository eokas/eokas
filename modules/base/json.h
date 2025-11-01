#pragma once
#include "./hom.h"

namespace eokas {
    struct JSON {
        static String stringify(const HomNode& json);
        static HomNode parse(const String& source);
    };
}
