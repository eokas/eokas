#include "Scene/CameraComponent.h"
#include "Scene/TransformComponent.h"
#include "Actors/Actor.h"

namespace eokas
{
    namespace
    {
        Vector3 rotateVector(const Quaternion& q, const Vector3& v)
        {
            Vector3 u(q.x, q.y, q.z);
            const f32_t w = q.w;
            return u * (2.0f * Vector3::dot(u, v))
                + v * (w * w - Vector3::dot(u, u))
                + Vector3::cross(u, v) * (2.0f * w);
        }

        void cameraPose(const CameraComponent* self, Vector3& position, Quaternion& rotation)
        {
            position = Vector3::ZERO;
            rotation = Quaternion::IDENTITY;
            if (self == nullptr)
            {
                return;
            }
            Actor* owner = self->actor();
            if (owner == nullptr)
            {
                return;
            }
            TransformComponent* transform = owner->getComponent<TransformComponent>();
            if (transform == nullptr)
            {
                return;
            }
            position = transform->globalPosition();
            rotation = transform->globalRotation();
        }
    }

    Matrix4 CameraComponent::viewMatrix() const
    {
        Vector3 position;
        Quaternion rotation;
        cameraPose(this, position, rotation);
        const Vector3 forward = rotateVector(rotation, Vector3::FORWARD);
        const Vector3 up = rotateVector(rotation, Vector3::UP);
        return Matrix4::lookToLH(position, forward, up);
    }

    Matrix4 CameraComponent::projectionMatrix() const
    {
        if (perspective)
        {
            return Matrix4::perspectiveFovLH(fovY, aspect, nearZ, farZ);
        }
        const f32_t height = orthoHeight;
        const f32_t width = height * aspect;
        return Matrix4::orthographicLH(width, height, nearZ, farZ);
    }

    Matrix4 CameraComponent::viewProjectionMatrix() const
    {
        return Matrix4::transform(viewMatrix(), projectionMatrix());
    }
}
