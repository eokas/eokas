#ifndef _EOKAS_UI_HEADER_H_
#define _EOKAS_UI_HEADER_H_

#include "base/main.h"
#include "gpu/main.h"

namespace eokas
{
    struct UIVertex
    {
        Vector2 position;
        Vector2 uv;
        Vector4 color;
    };
}

#endif//_EOKAS_UI_HEADER_H_
