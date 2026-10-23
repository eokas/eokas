#include "Actor.h"



namespace eokas

{

    Actor::~Actor()

    {

        if (mParent != nullptr)

        {

            mParent->detachChild(this);

        }

        for (Actor* child : mChildren)

        {

            if (child != nullptr && child->mParent == this)

            {

                child->mParent = nullptr;

            }

        }

    }



    bool Actor::isUnder(Actor* ancestor) const

    {

        for (const Actor* cursor = this; cursor != nullptr; cursor = cursor->mParent)

        {

            if (cursor == ancestor)

            {

                return true;

            }

        }

        return false;

    }



    bool Actor::attachChild(Actor* child)

    {

        if (child == nullptr || this->isUnder(child))

        {

            return false;

        }

        if (child->mParent == this)

        {

            return true;

        }

        if (child->mParent != nullptr)

        {

            child->mParent->detachChild(child);

        }

        child->mParent = this;

        mChildren.push_back(child);

        return true;

    }



    Actor* Actor::detachChild(Actor* child)

    {

        if (child == nullptr)

        {

            return nullptr;

        }

        for (auto it = mChildren.begin(); it != mChildren.end(); ++it)

        {

            if (*it == child)

            {

                mChildren.erase(it);

                if (child->mParent == this)

                {

                    child->mParent = nullptr;

                }

                return child;

            }

        }

        return nullptr;

    }



    void Actor::detachChildren()

    {

        for (Actor* child : mChildren)

        {

            if (child != nullptr && child->mParent == this)

            {

                child->mParent = nullptr;

            }

        }

        mChildren.clear();

    }

}

