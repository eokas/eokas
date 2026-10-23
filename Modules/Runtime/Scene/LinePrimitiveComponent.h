#ifndef _EOKAS_SCENE_LINEPRIMITIVECOMPONENT_H_
#define _EOKAS_SCENE_LINEPRIMITIVECOMPONENT_H_

#include "Actors/Component.h"

namespace eokas
{
    class LinePrimitiveComponent : public Component
    {
    public:
        std::vector<Vector3> points;
        Color color = Color(1.0f, 1.0f, 1.0f, 1.0f);
        f32_t width = 1.0f;
        bool closed = false;
        bool visible = true;

        Bounds3 localBounds() const;
    };
}

#endif//_EOKAS_SCENE_LINEPRIMITIVECOMPONENT_H_
