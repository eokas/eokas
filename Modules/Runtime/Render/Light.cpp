#include "./Light.h"
#include <cmath>

namespace eokas
{
    Vector3 Light::direction() const
    {
        return transform.forward();
    }

    void Light::fillGpu(LightUniforms& out) const
    {
        Vector3 dir = direction();
        out.colorIntensity[0] = color.r;
        out.colorIntensity[1] = color.g;
        out.colorIntensity[2] = color.b;
        out.colorIntensity[3] = intensity;
        out.posRange[0] = transform.position.x;
        out.posRange[1] = transform.position.y;
        out.posRange[2] = transform.position.z;
        out.posRange[3] = range;
        out.dirSpot[0] = dir.x;
        out.dirSpot[1] = dir.y;
        out.dirSpot[2] = dir.z;
        out.dirSpot[3] = cosf(outerCone);
        out.typeInner[0] = (float)(int)type;
        out.typeInner[1] = cosf(innerCone);
        out.typeInner[2] = 0.0f;
        out.typeInner[3] = 0.0f;
    }
}
