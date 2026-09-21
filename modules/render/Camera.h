#ifndef _EOKAS_RENDER_CAMERA_H_
#define _EOKAS_RENDER_CAMERA_H_

#include "./header.h"

namespace eokas
{
    struct Camera
    {
        using Ref = std::shared_ptr<Camera>;
        Transform transform;
        float fovY = Math::PI / 4.0f;
        float aspect = 16.0f / 9.0f;
        float nearZ = 0.1f;
        float farZ = 100.0f;
        bool perspective = true;
        float orthoHeight = 10.0f;
        Viewport viewport;

        Matrix4 viewMatrix() const;
        Matrix4 projectionMatrix() const;
        Matrix4 viewProjectionMatrix() const;
    };
}

#endif
