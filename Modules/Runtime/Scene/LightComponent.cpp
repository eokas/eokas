#include "Scene/LightComponent.h"
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

        void lightPose(const LightComponent* self, Vector3& position, Quaternion& rotation)
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

    Vector3 LightComponent::position() const
    {
        Vector3 position;
        Quaternion rotation;
        lightPose(this, position, rotation);
        return position;
    }

    Vector3 LightComponent::direction() const
    {
        Vector3 position;
        Quaternion rotation;
        lightPose(this, position, rotation);
        return rotateVector(rotation, Vector3::FORWARD);
    }
}
