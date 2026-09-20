#include "./Camera.h"

namespace eokas
{
    Matrix4 Camera::viewMatrix() const
    {
        if (useFocus)
            return Matrix4::lookAtLH(transform.position, focus, up);
        return Matrix4::lookToLH(transform.position, transform.forward(), transform.up());
    }

    Matrix4 Camera::projectionMatrix() const
    {
        if (perspective)
            return Matrix4::perspectiveFovLH(fovY, aspect, nearZ, farZ);
        float height = orthoHeight;
        float width = height * aspect;
        return Matrix4::orthographicLH(width, height, nearZ, farZ);
    }

    Matrix4 Camera::viewProjectionMatrix() const
    {
        return Matrix4::transform(viewMatrix(), projectionMatrix());
    }
}
