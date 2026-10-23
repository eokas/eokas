#ifndef _EOKAS_ACTORS_COMPONENT_H_
#define _EOKAS_ACTORS_COMPONENT_H_

#include "Base/Main.h"

namespace eokas
{
    class Actor;

    class Component
    {
        friend class Actor;

    public:
        Component() = default;
        virtual ~Component();

        const String& name() const { return mName; }
        void setName(const String& name) { mName = name; }

        virtual void init();
        virtual void start();
        virtual void tick(float deltaTime);
        virtual void quit();

        Actor* actor();
        Actor* actor() const { return mActor; }

    private:
        void setActor(Actor* actor);
        String mName;
        Actor* mActor = nullptr;
    };
}

#endif//_EOKAS_ACTORS_COMPONENT_H_
