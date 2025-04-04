
#ifndef  _EOKAS_BASE_JSON_H_
#define  _EOKAS_BASE_JSON_H_

#include "./hom.h"

namespace eokas {
    struct JSON {
        static String stringify(const HomNode& json);
        static HomNode parse(const String& source);
    };
}

#endif//_EOKAS_BASE_JSON_H_
