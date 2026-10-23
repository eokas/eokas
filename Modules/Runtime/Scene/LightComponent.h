#ifndef _EOKAS_SCENE_LIGHTCOMPONENT_H_
#define _EOKAS_SCENE_LIGHTCOMPONENT_H_

#include "Actors/Component.h"

namespace eokas
{
    class LightComponent : public Component
    {
    public:
        enum class LightType
        {
            Directional = 0,
            Point = 1,
            Spot = 2
        };

        LightType type = LightType::Directional;
        Color color = Color(1.0f, 1.0f, 1.0f, 1.0f);
        f32_t intensity = 1.0f;
        f32_t range = 10.0f;
        f32_t innerCone = 0.4f;
        f32_t outerCone = 0.7f;
        bool enabled = true;

        Vector3 position() const;
        Vector3 direction() const;
    };
}

#endif//_EOKAS_SCENE_LIGHTCOMPONENT_H_
