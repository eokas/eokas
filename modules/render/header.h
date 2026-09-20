#ifndef _EOKAS_RENDER_HEADER_H_
#define _EOKAS_RENDER_HEADER_H_

#include "base/main.h"
#include "gpu/main.h"
#include "mesh/GeoMesh.h"
#include "mesh/Skeleton.h"

namespace eokas
{
    constexpr uint32_t kMaxSpaceLights = 4;

    struct Transform
    {
        Vector3 position = Vector3(0, 0, 0);
        Quaternion rotation = Quaternion::IDENTITY;
        Vector3 scale = Vector3(1, 1, 1);

        Matrix4 toMatrix() const;
        Vector3 right() const;
        Vector3 up() const;
        Vector3 forward() const;
    };

    struct TransformUniforms
    {
        Matrix4 world;
        Matrix4 wvp;
        float cameraPos[4];
    };

    struct LightUniforms
    {
        float colorIntensity[4];
        float posRange[4];
        float dirSpot[4];
        float typeInner[4];
    };

    struct LightingUniforms
    {
        float ambientAndCount[4];
        LightUniforms lights[kMaxSpaceLights];
    };

    struct MaterialUniforms
    {
        float albedo[4];
        float metallic;
        float roughness;
        float ao;
        float _pad0;
        float emissive[4];
    };
}

#endif
