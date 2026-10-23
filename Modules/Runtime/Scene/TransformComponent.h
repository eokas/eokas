#ifndef _EOKAS_SCENE_TRANSFORMCOMPONENT_H_
#define _EOKAS_SCENE_TRANSFORMCOMPONENT_H_

#include "Actors/Component.h"

namespace eokas
{
    class TransformComponent : public Component
    {
    public:
        Vector3 localPosition = Vector3::ZERO;
        Quaternion localRotation = Quaternion::IDENTITY;
        Vector3 localScale = Vector3::ONE;

        Vector3 globalPosition() const;
        Quaternion globalRotation() const;
        Vector3 globalScale() const;
    };
}

#endif//_EOKAS_SCENE_TRANSFORMCOMPONENT_H_
