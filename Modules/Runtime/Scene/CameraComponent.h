#ifndef _EOKAS_SCENE_CAMERACOMPONENT_H_
#define _EOKAS_SCENE_CAMERACOMPONENT_H_

#include "Actors/Component.h"

namespace eokas
{
    class CameraComponent : public Component
    {
    public:
        f32_t fovY = Math::PI / 4.0f;
        f32_t aspect = 16.0f / 9.0f;
        f32_t nearZ = 0.1f;
        f32_t farZ = 100.0f;
        bool perspective = true;
        f32_t orthoHeight = 10.0f;

        Matrix4 viewMatrix() const;
        Matrix4 projectionMatrix() const;
        Matrix4 viewProjectionMatrix() const;
    };
}

#endif//_EOKAS_SCENE_CAMERACOMPONENT_H_
