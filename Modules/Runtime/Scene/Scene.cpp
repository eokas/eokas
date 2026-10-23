#include "Scene.h"

namespace eokas
{
    Scene::~Scene()
    {
        clear();
    }

    Actor* Scene::spawnActor()
    {
        auto actor = std::make_unique<Actor>();
        Actor* ptr = actor.get();
        mActors.push_back(std::move(actor));
        return ptr;
    }

    void Scene::clear()
    {
        mActors.clear();
    }
}
