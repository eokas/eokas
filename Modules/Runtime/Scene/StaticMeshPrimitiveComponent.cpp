#include "Scene/StaticMeshPrimitiveComponent.h"

namespace eokas
{
    Bounds3 StaticMeshPrimitiveComponent::localBounds() const
    {
        Bounds3 bounds;
        if (mesh.vertices.empty())
        {
            return bounds;
        }
        bounds.min = mesh.vertices[0].position;
        bounds.max = mesh.vertices[0].position;
        for (size_t i = 1; i < mesh.vertices.size(); ++i)
        {
            bounds.expand(mesh.vertices[i].position);
        }
        return bounds;
    }
}
