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

        TransformComponent* parentTransform(const TransformComponent* self)
        {
            if (self == nullptr)
            {
                return nullptr;
            }
            Actor* owner = self->actor();
            if (owner == nullptr)
            {
                return nullptr;
            }
            Actor* parentActor = owner->parent();
            if (parentActor == nullptr)
            {
                return nullptr;
            }
            return parentActor->getComponent<TransformComponent>();
        }
    }

    Vector3 TransformComponent::globalScale() const
    {
        TransformComponent* parent = parentTransform(this);
        if (parent == nullptr)
        {
            return localScale;
        }
        return parent->globalScale() * localScale;
    }

    Quaternion TransformComponent::globalRotation() const
    {
        TransformComponent* parent = parentTransform(this);
        if (parent == nullptr)
        {
            return localRotation;
        }
        return parent->globalRotation() * localRotation;
    }

    Vector3 TransformComponent::globalPosition() const
    {
        TransformComponent* parent = parentTransform(this);
        if (parent == nullptr)
        {
            return localPosition;
        }
        const Vector3 parentScale = parent->globalScale();
        const Vector3 scaledLocal = localPosition * parentScale;
        return parent->globalPosition() + rotateVector(parent->globalRotation(), scaledLocal);
    }
}
