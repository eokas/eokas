#include "Component.h"
#include "Actor.h"

namespace eokas
{
    Component::~Component() = default;

    void Component::init()
    {
    }

    void Component::start()
    {
    }

    void Component::tick(float deltaTime)
    {
        (void)deltaTime;
    }

    void Component::quit()
    {
    }

    Actor* Component::actor()
    {
        return mActor;
    }

    void Component::setActor(Actor* actor)
    {
        mActor = actor;
    }
}
