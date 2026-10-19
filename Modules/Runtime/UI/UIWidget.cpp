#include "UIWidget.h"

namespace eokas
{
    UIWidget::~UIWidget()
    {
        for (auto& child : mChildren)
        {
            if (child && child->mParent == this)
            {
                child->mParent = nullptr;
            }
        }
    }

    bool UIWidget::isUnder(const UIWidget* ancestor) const
    {
        for (const UIWidget* cursor = this; cursor != nullptr; cursor = cursor->mParent)
        {
            if (cursor == ancestor)
            {
                return true;
            }
        }
        return false;
    }

    UIWidget::Ref UIWidget::attachChild(UIWidget::Ref child)
    {
        if (!child || this->isUnder(child.get()))
        {
            return nullptr;
        }
        if (child->mParent == this)
        {
            return child;
        }
        if (child->mParent != nullptr)
        {
            child->mParent->detachChild(child.get());
        }
        child->mParent = this;
        mChildren.push_back(child);
        return child;
    }

    UIWidget::Ref UIWidget::detachChild(UIWidget* child)
    {
        if (child == nullptr)
        {
            return nullptr;
        }
        for (auto it = mChildren.begin(); it != mChildren.end(); ++it)
        {
            if (it->get() == child)
            {
                Ref held = *it;
                mChildren.erase(it);
                if (held->mParent == this)
                {
                    held->mParent = nullptr;
                }
                return held;
            }
        }
        return nullptr;
    }

    void UIWidget::detachChildren()
    {
        for (auto& child : mChildren)
        {
            if (child && child->mParent == this)
            {
                child->mParent = nullptr;
            }
        }
        mChildren.clear();
    }

    Matrix3 UIWidget::matrixLocalToScreen() const
    {
        Matrix3 local = shape.localTrans();
        if (mParent == nullptr)
        {
            return local;
        }
        return Matrix3::transform(local, mParent->matrixPivotToScreen());
    }

    Matrix3 UIWidget::matrixScreenToLocal() const
    {
        return this->matrixLocalToScreen().inverse();
    }

    Matrix3 UIWidget::matrixPivotToScreen() const
    {
        Matrix3 shift = Matrix3::translation(Vector2(shape.pivot.x * shape.size.x, shape.pivot.y * shape.size.y));
        return Matrix3::transform(shift, this->matrixLocalToScreen());
    }

    Matrix3 UIWidget::matrixScreenToPivot() const
    {
        return this->matrixPivotToScreen().inverse();
    }

    Vector2 UIWidget::screenToPivot(const Vector2& screen) const
    {
        return UIShape::transformPoint(this->matrixScreenToPivot(), screen);
    }

    Vector2 UIWidget::pivotToScreen(const Vector2& pivot) const
    {
        return UIShape::transformPoint(this->matrixPivotToScreen(), pivot);
    }

    Vector2 UIWidget::localToScreen(const Vector2& local) const
    {
        return UIShape::transformPoint(this->matrixLocalToScreen(), local);
    }

    Vector2 UIWidget::screenToLocal(const Vector2& screen) const
    {
        return UIShape::transformPoint(this->matrixScreenToLocal(), screen);
    }

    Rect UIWidget::screenBounds() const
    {
        return shape.bounds(this->matrixLocalToScreen());
    }

    void UIWidget::addQuad(UIPrimitive& primitive, const Rect& local, const Rect& uv, const Color& color) const
    {
        primitive.addQuad(this->matrixLocalToScreen(), local, uv, color);
    }

    void UIWidget::layout()
    {
        if (!visible)
        {
            return;
        }
        for (auto& child : mChildren)
        {
            if (child)
            {
                child->layout();
            }
        }
    }

    void UIWidget::render(UIPrimitive& primitive)
    {
        if (!visible)
        {
            return;
        }
        for (auto& child : mChildren)
        {
            if (!child || child->floating)
            {
                continue;
            }
            if (!primitive.outsideClip(child->shape.bounds(child->matrixLocalToScreen())))
            {
                child->render(primitive);
            }
        }
        for (auto& child : mChildren)
        {
            if (child && child->floating)
            {
                child->render(primitive);
            }
        }
    }

    bool UIWidget::contains(const Vector2& point) const
    {
        return point.x >= 0.0f && point.y >= 0.0f
            && point.x <= shape.size.x && point.y <= shape.size.y;
    }

    UIWidget* UIWidget::pick(const Vector2& point)
    {
        if (!visible || shape.scale.x == 0.0f || shape.scale.y == 0.0f)
        {
            return nullptr;
        }
        Vector2 local = shape.pivotToLocal(point);
        bool inside = this->contains(local);
        Vector2 childPoint = local - Vector2(shape.pivot.x * shape.size.x, shape.pivot.y * shape.size.y);
        for (auto it = mChildren.rbegin(); it != mChildren.rend(); ++it)
        {
            if (*it && (*it)->floating)
            {
                if (UIWidget* hit = (*it)->pick(childPoint))
                {
                    return hit;
                }
            }
        }
        for (auto it = mChildren.rbegin(); it != mChildren.rend(); ++it)
        {
            if (*it && !(*it)->floating)
            {
                if (UIWidget* hit = (*it)->pick(childPoint))
                {
                    return hit;
                }
            }
        }
        if (pickable && inside)
        {
            return this;
        }
        return nullptr;
    }

    namespace
    {
        Vector2 screenToParentPivot(const UIWidget& widget, const Vector2& screen)
        {
            if (widget.parent() == nullptr)
            {
                return screen;
            }
            return widget.parent()->screenToPivot(screen);
        }
    }

    bool UIWidget::handlePointerEnter(float screenX, float screenY)
    {
        (void)screenX;
        (void)screenY;
        mHovered = true;
        if (onPointerEnter)
        {
            onPointerEnter();
            return true;
        }
        return false;
    }

    bool UIWidget::handlePointerLeave(float screenX, float screenY)
    {
        (void)screenX;
        (void)screenY;
        mHovered = false;
        if (onPointerLeave)
        {
            onPointerLeave();
            return true;
        }
        return false;
    }

    bool UIWidget::handlePointerMove(float screenX, float screenY, const Vector2& delta)
    {
        if (onPointerMove)
        {
            Vector2 position = screenToParentPivot(*this, Vector2(screenX, screenY));
            onPointerMove(position, delta);
            return true;
        }
        return false;
    }

    bool UIWidget::handlePointerPress(float screenX, float screenY, int button)
    {
        (void)screenX;
        (void)screenY;
        (void)button;
        mPressed = true;
        if (onPointerPress)
        {
            onPointerPress();
            return true;
        }
        return false;
    }

    bool UIWidget::handlePointerRelease(float screenX, float screenY, int button)
    {
        (void)screenX;
        (void)screenY;
        (void)button;
        mPressed = false;
        if (onPointerRelease)
        {
            onPointerRelease();
            return true;
        }
        return false;
    }

    void UIWidget::handleFocusGain()
    {
        if (mFocused)
        {
            return;
        }
        mFocused = true;
        if (onGotFocus)
        {
            onGotFocus();
        }
    }

    void UIWidget::handleFocusLoss()
    {
        if (!mFocused)
        {
            return;
        }
        mFocused = false;
        if (onLostFocus)
        {
            onLostFocus();
        }
    }

    bool UIWidget::handleClick(float screenX, float screenY)
    {
        (void)screenX;
        (void)screenY;
        constexpr auto interval = std::chrono::milliseconds(500);
        auto now = std::chrono::steady_clock::now();
        if (mLastClickTime.has_value() && now - *mLastClickTime < interval)
        {
            mLastClickTime.reset();
            return this->handleDoubleClick(screenX, screenY);
        }
        mLastClickTime = now;
        if (onClick)
        {
            onClick();
            return true;
        }
        return false;
    }

    bool UIWidget::handleDoubleClick(float screenX, float screenY)
    {
        (void)screenX;
        (void)screenY;
        if (onDoubleClick)
        {
            onDoubleClick();
            return true;
        }
        return false;
    }

    bool UIWidget::handleDrag(float screenX, float screenY, const Vector2& delta)
    {
        (void)screenX;
        (void)screenY;
        if (onDrag)
        {
            Vector2 position = screenToParentPivot(*this, Vector2(screenX, screenY));
            onDrag(position, delta);
            return true;
        }
        return false;
    }

    bool UIWidget::handleDrop(float screenX, float screenY, UIWidget* hitUnderCursor, const Vector2& delta)
    {
        (void)hitUnderCursor;
        if (onDrop)
        {
            Vector2 position = screenToParentPivot(*this, Vector2(screenX, screenY));
            onDrop(position, delta);
            return true;
        }
        return false;
    }

    bool UIWidget::handleKeyPress(const UIKey& key, const UIKeyMods& mods)
    {
        if (onKeyPress)
        {
            onKeyPress(key, mods);
            return true;
        }
        return false;
    }

    bool UIWidget::handleKeyRelease(const UIKey& key, const UIKeyMods& mods)
    {
        if (onKeyRelease)
        {
            onKeyRelease(key, mods);
            return true;
        }
        return false;
    }

    bool UIWidget::handleWheel(float screenX, float screenY, float deltaX, float deltaY)
    {
        if (!visible || shape.scale.x == 0.0f || shape.scale.y == 0.0f)
        {
            return false;
        }
        Vector2 point(screenX, screenY);
        for (auto it = mChildren.rbegin(); it != mChildren.rend(); ++it)
        {
            if (*it && (*it)->floating && (*it)->handleWheel(screenX, screenY, deltaX, deltaY))
            {
                return true;
            }
        }
        for (auto it = mChildren.rbegin(); it != mChildren.rend(); ++it)
        {
            if (*it && !(*it)->floating && (*it)->handleWheel(screenX, screenY, deltaX, deltaY))
            {
                return true;
            }
        }
        if (UIWidget* hit = this->pick(screenToParentPivot(*this, point)))
        {
            if (hit != this && hit->handleWheel(screenX, screenY, deltaX, deltaY))
            {
                return true;
            }
            Vector2 parentPivot = screenToParentPivot(*hit, point);
            if (hit->onWheel)
            {
                hit->onWheel(parentPivot, deltaY);
                return true;
            }
        }
        return false;
    }

    void UIWidget::endActiveDrag()
    {
    }

    bool UIWidget::containsDescendant(const UIWidget* target) const
    {
        if (target == nullptr)
        {
            return false;
        }
        if (this == target)
        {
            return true;
        }
        for (auto& child : mChildren)
        {
            if (child && child->containsDescendant(target))
            {
                return true;
            }
        }
        return false;
    }

    void UIWidget::resetPointerStateRecursive()
    {
        this->endActiveDrag();
        const bool active = mHovered || mPressed;
        mHovered = false;
        mPressed = false;
        if (active)
        {
            this->handlePointerRelease(0.0f, 0.0f, 0);
        }
        for (auto& child : mChildren)
        {
            if (child)
            {
                child->resetPointerStateRecursive();
            }
        }
    }

    void UIWidget::resize(const Vector2& newSize)
    {
        Vector2 shift = Vector2(
            shape.pivot.x * (newSize.x - shape.size.x),
            shape.pivot.y * (newSize.y - shape.size.y));
        shape.origin += UIShape::transformVector(shape.localTrans(), shift);
        shape.size = newSize;
        for (auto& child : mChildren)
        {
            if (child)
            {
                child->shape.origin -= shift;
            }
        }
    }

    void UIWidget::placeChild(UIWidget& child, const Vector2& topLeftLocal)
    {
        Vector2 topLeftPivot = topLeftLocal - Vector2(shape.pivot.x * shape.size.x, shape.pivot.y * shape.size.y);
        child.shape.setBox(topLeftPivot, child.shape.size);
    }

    void UIWidget::placeChild(UIWidget& child, const Vector2& topLeftLocal, const Vector2& childSize)
    {
        child.shape.size = childSize;
        this->placeChild(child, topLeftLocal);
    }
}
