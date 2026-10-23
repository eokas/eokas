#ifndef _EOKAS_SCENE_STATICMESHPRIMITIVECOMPONENT_H_
#define _EOKAS_SCENE_STATICMESHPRIMITIVECOMPONENT_H_

#include "Actors/Component.h"
#include "Mesh/GeoMesh.h"

namespace eokas
{
    class StaticMeshPrimitiveComponent : public Component
    {
    public:
        GeoMesh mesh;
        bool visible = true;
        bool castShadow = false;
        bool receiveShadow = false;

        Bounds3 localBounds() const;
    };
}

#endif//_EOKAS_SCENE_STATICMESHPRIMITIVECOMPONENT_H_
