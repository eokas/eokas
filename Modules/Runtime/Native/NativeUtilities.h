#ifndef _EOKAS_NATIVE_UTILITIES_H_
#define _EOKAS_NATIVE_UTILITIES_H_

#include "./Header.h"

namespace eokas
{
    struct NativeUtilities
    {
        // Samples the screen pixel under the cursor.
        // Returns alpha < 0 when the pixel cannot be read.
        // Alpha on a successful sample is 1.
        static Color sampleScreenColor();
    };
}

#endif//_EOKAS_NATIVE_UTILITIES_H_
