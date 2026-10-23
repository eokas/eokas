#include "Scene/LinePrimitiveComponent.h"

namespace eokas
{
    Bounds3 LinePrimitiveComponent::localBounds() const
    {
        Bounds3 bounds;
        if (points.empty())
        {
            return bounds;
        }
        bounds.min = points[0];
        bounds.max = points[0];
        for (size_t i = 1; i < points.size(); ++i)
        {
            bounds.expand(points[i]);
        }
        return bounds;
    }
}
