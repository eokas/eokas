#ifndef _EOKAS_RENDER_CAMERA_H_
#define _EOKAS_RENDER_CAMERA_H_

#include "./header.h"

namespace eokas
{
    struct Camera
    {
        using Ref = std::shared_ptr<Camera>;
        Transform transform;
        Vector3 focus = Vector3(0, 0, 0);
        Vector3 up = Vector3(0, 1, 0);
        bool useFocus = true;
        float fovY = Math::PI / 4.0f;
        float aspect = 16.0f / 9.0f;
        float nearZ = 0.1f;
        float farZ = 100.0f;
        bool perspective = true;
        float orthoHeight = 10.0f;

        Matrix4 viewMatrix() const;
        Matrix4 projectionMatrix() const;
        Matrix4 viewProjectionMatrix() const;
    };
}

#endif
