#ifndef _EOKAS_ACTORS_ACTOR_H_

#define _EOKAS_ACTORS_ACTOR_H_



#include "Actors/Component.h"

#include <memory>

#include <type_traits>

#include <utility>

#include <vector>



namespace eokas

{

    class Actor

    {

    public:

        Actor() = default;

        ~Actor();



        Actor(const Actor&) = delete;

        Actor& operator=(const Actor&) = delete;



        Actor* parent() const { return mParent; }

        const std::vector<Actor*>& children() const { return mChildren; }



        bool attachChild(Actor* child);

        Actor* detachChild(Actor* child);

        void detachChildren();



        template<typename T, typename... Args>

        T* addComponent(Args&&... args)

        {

            static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");

            auto component = std::make_unique<T>(std::forward<Args>(args)...);

            T* ptr = component.get();

            ptr->setActor(this);

            mComponents.push_back(std::move(component));

            return ptr;

        }



        template<typename T>

        T* getComponent()

        {

            static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");

            for (auto& c : mComponents)

            {

                if (T* t = dynamic_cast<T*>(c.get()))

                {

                    return t;

                }

            }

            return nullptr;

        }



    private:

        bool isUnder(Actor* ancestor) const;



        Actor* mParent = nullptr;

        std::vector<Actor*> mChildren;

        std::vector<std::unique_ptr<Component>> mComponents;

    };

}



#endif//_EOKAS_ACTORS_ACTOR_H_

