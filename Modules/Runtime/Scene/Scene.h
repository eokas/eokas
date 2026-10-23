#ifndef _EOKAS_SCENE_SCENE_H_
#define _EOKAS_SCENE_SCENE_H_

#include "Actors/Actor.h"
#include <cstddef>
#include <memory>
#include <vector>

namespace eokas
{
    class Scene
    {
    public:
        Scene() = default;
        ~Scene();

        Actor* spawnActor();
        std::size_t actorCount() const { return mActors.size(); }
        void clear();

    private:
        std::vector<std::unique_ptr<Actor>> mActors;
    };
}

#endif//_EOKAS_SCENE_SCENE_H_
