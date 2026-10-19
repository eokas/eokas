#include "./Space.h"
#include <algorithm>

namespace eokas
{
    void Space::add(Camera::Ref camera) { if (camera) cameras.push_back(camera); }
    void Space::add(Light::Ref light) { if (light) lights.push_back(light); }
    void Space::add(Primitive::Ref primitive) { if (primitive) primitives.push_back(primitive); }

    void Space::remove(Camera::Ref camera)
    {
        cameras.erase(std::remove(cameras.begin(), cameras.end(), camera), cameras.end());
        if (activeCamera == camera) activeCamera = nullptr;
    }
    void Space::remove(Light::Ref light)
    {
        lights.erase(std::remove(lights.begin(), lights.end(), light), lights.end());
    }
    void Space::remove(Primitive::Ref primitive)
    {
        primitives.erase(std::remove(primitives.begin(), primitives.end(), primitive), primitives.end());
    }
}
