#ifndef _EOKAS_SCENE_SKELETALMESHPRIMITIVECOMPONENT_H_
#define _EOKAS_SCENE_SKELETALMESHPRIMITIVECOMPONENT_H_

#include "Actors/Component.h"
#include "Mesh/GeoMesh.h"
#include "Mesh/Skeleton.h"

namespace eokas
{
    class SkeletalMeshPrimitiveComponent : public Component
    {
    public:
        GeoMesh mesh;
        Skeleton skeleton;
        Skeleton::Pose pose;
        bool visible = true;
        bool castShadow = false;
        bool receiveShadow = false;

        Bounds3 localBounds() const;
    };
}

#endif//_EOKAS_SCENE_SKELETALMESHPRIMITIVECOMPONENT_H_
