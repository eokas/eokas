#ifndef _EOKAS_RENDER_LIGHT_H_
#define _EOKAS_RENDER_LIGHT_H_

#include "./header.h"

namespace eokas
{
    enum class LightType
    {
        Directional = 0,
        Point = 1,
        Spot = 2
    };

    struct Light
    {
        using Ref = std::shared_ptr<Light>;
        LightType type = LightType::Directional;
        Transform transform;
        Color color = Color(1, 1, 1, 1);
        float intensity = 1.0f;
        float range = 10.0f;
        float innerCone = 0.4f;
        float outerCone = 0.7f;
        bool enabled = true;

        Vector3 direction() const;
        void fillGpu(LightUniforms& out) const;
    };
}

#endif
