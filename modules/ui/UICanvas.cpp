#include "UICanvas.h"
#include "UIView.h"

#include <cstddef>

namespace eokas
{
    namespace
    {
        constexpr const char* kUIShaderPath = "../shaders/UI.hlsl";
    }

    void UICanvas::init(uint32_t width, uint32_t height)
    {
        mWidth = (float)width;
        mHeight = (float)height;
        mHovered = nullptr;
        mPressed = nullptr;
        mPressedButton = -1;
        mFocused = nullptr;

        if (!mRoot)
        {
            mRoot = std::make_shared<UIWidget>();
        }

        mShape = std::make_shared<UIShape>();
        mShape->material = std::make_shared<Material>();
        mShape->material->setShaderPath(kUIShaderPath);
        mShape->material->addVertexElement({"POSITION", 0, (uint32_t)offsetof(UIVertex, position), Format::R32G32_FLOAT});
        mShape->material->addVertexElement({"TEXCOORD", 0, (uint32_t)offsetof(UIVertex, uv), Format::R32G32_FLOAT});
        mShape->material->addVertexElement({"COLOR", 0, (uint32_t)offsetof(UIVertex, color), Format::R32G32B32A32_FLOAT});
        mShape->material->setCullMode(CullMode::None);
        DepthStencilState depthStencil;
        depthStencil.depthTest = false;
        depthStencil.depthWrite = false;
        depthStencil.depthFunc = CompareOp::Always;
        mShape->material->setDepthStencilState(depthStencil);
        SamplerState sampler;
        sampler.minFilter = SamplerFilterMode::Linear;
        sampler.magFilter = SamplerFilterMode::Linear;
        sampler.mipFilter = SamplerFilterMode::Point;
        mShape->material->setSamplerState(0, sampler);
        BlendState blend;
        blend.enabled = true;
        blend.srcColor = BlendFactor::SrcAlpha;
        blend.dstColor = BlendFactor::OneMinusSrcAlpha;
        blend.srcAlpha = BlendFactor::One;
        blend.dstAlpha = BlendFactor::OneMinusSrcAlpha;
        mShape->material->setBlendState(blend);
    }

    void UICanvas::quit()
    {
        this->setFocus(nullptr);
        this->resetPointerState(mRoot.get());
        mHovered = nullptr;
        mPressed = nullptr;
        mPressedButton = -1;
        mRoot.reset();
        if (mShape)
        {
            mShape->material.reset();
        }
        mShape.reset();
    }

    const std::shared_ptr<UIWidget>& UICanvas::root() const
    {
        return mRoot;
    }

    void UICanvas::setRoot(const std::shared_ptr<UIWidget>& widget)
    {
        this->setFocus(nullptr);
        this->resetPointerState(mRoot.get());
        mHovered = nullptr;
        mPressed = nullptr;
        mPressedButton = -1;
        mRoot = widget;
    }

    void UICanvas::flush()
    {
        if (!mShape)
        {
            return;
        }

        mShape->begin();
        if (mRoot)
        {
            mRoot->layout(mRoot->rect);
            mRoot->render(*mShape);
        }
        mShape->end();
    }

    UIShape::Ref UICanvas::shape() const
    {
        return mShape;
    }

    UIWidget* UICanvas::hitTest(float x, float y)
    {
        if (mRoot)
        {
            mRoot->layout(mRoot->rect);
        }
        return this->hitTestNode(mRoot.get(), x, y, 0.0f, 0.0f);
    }

    void UICanvas::onMouseMove(float x, float y)
    {
        if (mPressed != nullptr)
        {
            this->dispatchDrag(x, y);
        }

        UIWidget* hit = this->hitTest(x, y);
        if (hit == mHovered)
        {
            return;
        }
        if (mHovered != nullptr)
        {
            mHovered->triggerPointerLeave();
        }
        mHovered = hit;
        if (mHovered != nullptr)
        {
            mHovered->triggerPointerEnter();
        }
    }

    void UICanvas::onMouseDown(float x, float y, int button)
    {
        this->onMouseMove(x, y);
        if (mPressed != nullptr || mHovered == nullptr)
        {
            if (mPressed == nullptr && button == 0)
            {
                this->setFocus(nullptr);
            }
            return;
        }
        mPressed = mHovered;
        mPressedButton = button;
        if (button == 0)
        {
            bool insideFocus = mFocused != nullptr && this->containsWidget(mFocused, mPressed);
            if (mPressed->acceptsKeyFocus())
            {
                this->setFocus(mPressed);
            }
            else if (!insideFocus)
            {
                this->setFocus(nullptr);
            }
        }
        mPressed->triggerPointerPress();
        this->dispatchDrag(x, y);
    }

    void UICanvas::onMouseUp(float x, float y, int button)
    {
        UIWidget* hit = this->hitTest(x, y);
        UIWidget* pressed = mPressed;
        if (pressed != nullptr && button == mPressedButton)
        {
            pressed->triggerPointerRelease();
            if (hit == pressed && button == 0)
            {
                pressed->triggerClick();
            }
        }
        mPressed = nullptr;
        mPressedButton = -1;
        this->onMouseMove(x, y);
    }

    void UICanvas::onMouseWheel(float x, float y, float deltaX, float deltaY)
    {
        this->routeWheel(mRoot.get(), x, y, 0.0f, 0.0f, deltaX, deltaY);
    }

    void UICanvas::dispatchDrag(float x, float y)
    {
        if (mPressed == nullptr)
        {
            return;
        }
        float ox = 0.0f;
        float oy = 0.0f;
        this->findWidget(mRoot.get(), mPressed, 0.0f, 0.0f, ox, oy);
        mPressed->triggerPointerDrag(x - ox, y - oy, mPressedButton);
    }

    bool UICanvas::findWidget(UIWidget* node, UIWidget* target, float originX, float originY, float& outX, float& outY) const
    {
        if (node == nullptr)
        {
            return false;
        }
        if (node == target)
        {
            outX = originX;
            outY = originY;
            return true;
        }
        if (UIView* view = dynamic_cast<UIView*>(node))
        {
            const std::shared_ptr<UIWidget>& content = view->root();
            float cx = originX + content->rect.x;
            float cy = originY + content->rect.y;
            for (auto& child : content->children)
            {
                if (this->findWidget(child.get(), target, cx, cy, outX, outY))
                {
                    return true;
                }
            }
            return false;
        }
        for (auto& child : node->children)
        {
            if (this->findWidget(child.get(), target, originX, originY, outX, outY))
            {
                return true;
            }
        }
        return false;
    }

    bool UICanvas::routeWheel(UIWidget* widget, float x, float y, float originX, float originY, float deltaX, float deltaY)
    {
        if (widget == nullptr || !widget->visible)
        {
            return false;
        }

        if (UIView* view = dynamic_cast<UIView*>(widget))
        {
            const std::shared_ptr<UIWidget>& content = view->root();
            float cx = originX + content->rect.x;
            float cy = originY + content->rect.y;
            for (auto it = content->children.rbegin(); it != content->children.rend(); ++it)
            {
                if (*it && (*it)->floating && this->routeWheel(it->get(), x, y, cx, cy, deltaX, deltaY))
                {
                    return true;
                }
            }
            for (auto it = content->children.rbegin(); it != content->children.rend(); ++it)
            {
                if (*it && !(*it)->floating && this->routeWheel(it->get(), x, y, cx, cy, deltaX, deltaY))
                {
                    return true;
                }
            }
            Vector2 local(x - originX, y - originY);
            if (!view->rect.contains(local))
            {
                return false;
            }
            return view->scrollBy(deltaX, deltaY);
        }

        for (auto it = widget->children.rbegin(); it != widget->children.rend(); ++it)
        {
            if (*it && (*it)->floating && this->routeWheel(it->get(), x, y, originX, originY, deltaX, deltaY))
            {
                return true;
            }
        }
        for (auto it = widget->children.rbegin(); it != widget->children.rend(); ++it)
        {
            if (*it && !(*it)->floating && this->routeWheel(it->get(), x, y, originX, originY, deltaX, deltaY))
            {
                return true;
            }
        }
        return false;
    }

    void UICanvas::onChar(uint32_t codepoint)
    {
        if (!this->focusAlive())
        {
            return;
        }
        mFocused->triggerChar(codepoint);
    }

    void UICanvas::onKeyDown(UIKey key, const UIKeyMods& mods)
    {
        if (key == UIKey::Escape)
        {
            this->setFocus(nullptr);
            return;
        }
        if (!this->focusAlive())
        {
            return;
        }
        mFocused->triggerKey(key, mods);
    }

    void UICanvas::setFocus(UIWidget* widget)
    {
        if (mFocused == widget)
        {
            return;
        }
        if (mFocused != nullptr)
        {
            mFocused->triggerBlur();
        }
        mFocused = widget;
        if (mFocused != nullptr)
        {
            mFocused->triggerFocus();
        }
    }

    UIWidget* UICanvas::focus() const
    {
        return mFocused;
    }

    UIWidget* UICanvas::hitTestNode(UIWidget* widget, float x, float y, float originX, float originY)
    {
        if (widget == nullptr || !widget->visible)
        {
            return nullptr;
        }

        if (UIView* view = dynamic_cast<UIView*>(widget))
        {
            const std::shared_ptr<UIWidget>& content = view->root();
            float cx = originX + content->rect.x;
            float cy = originY + content->rect.y;
            for (auto it = content->children.rbegin(); it != content->children.rend(); ++it)
            {
                if (*it && (*it)->floating)
                {
                    UIWidget* hit = this->hitTestNode(it->get(), x, y, cx, cy);
                    if (hit != nullptr)
                    {
                        return hit;
                    }
                }
            }

            Vector2 local(x - originX, y - originY);
            if (view->scrollbarContains(local.x, local.y))
            {
                return view;
            }
            Rect vp = view->viewport();
            if (!vp.contains(local))
            {
                if (widget->interactive && widget->rect.contains(local))
                {
                    return widget;
                }
                return nullptr;
            }
            for (auto it = content->children.rbegin(); it != content->children.rend(); ++it)
            {
                if (*it && !(*it)->floating)
                {
                    UIWidget* hit = this->hitTestNode(it->get(), x, y, cx, cy);
                    if (hit != nullptr)
                    {
                        return hit;
                    }
                }
            }
            if (widget->interactive && widget->rect.contains(local))
            {
                return widget;
            }
            return nullptr;
        }

        for (auto it = widget->children.rbegin(); it != widget->children.rend(); ++it)
        {
            if (*it && (*it)->floating)
            {
                UIWidget* hit = this->hitTestNode(it->get(), x, y, originX, originY);
                if (hit != nullptr)
                {
                    return hit;
                }
            }
        }

        for (auto it = widget->children.rbegin(); it != widget->children.rend(); ++it)
        {
            if (*it && !(*it)->floating)
            {
                UIWidget* hit = this->hitTestNode(it->get(), x, y, originX, originY);
                if (hit != nullptr)
                {
                    return hit;
                }
            }
        }

        Vector2 local(x - originX, y - originY);
        if (widget->interactive && widget->rect.contains(local))
        {
            return widget;
        }
        return nullptr;
    }

    void UICanvas::resetPointerState(UIWidget* widget)
    {
        if (widget == nullptr)
        {
            return;
        }
        widget->resetPointerState();
        if (UIView* view = dynamic_cast<UIView*>(widget))
        {
            for (auto& child : view->root()->children)
            {
                this->resetPointerState(child.get());
            }
            return;
        }
        for (auto& child : widget->children)
        {
            this->resetPointerState(child.get());
        }
    }

    bool UICanvas::containsWidget(UIWidget* node, UIWidget* target) const
    {
        if (node == nullptr || target == nullptr)
        {
            return false;
        }
        if (node == target)
        {
            return true;
        }
        if (UIView* view = dynamic_cast<UIView*>(node))
        {
            for (auto& child : view->root()->children)
            {
                if (this->containsWidget(child.get(), target))
                {
                    return true;
                }
            }
            return false;
        }
        for (auto& child : node->children)
        {
            if (this->containsWidget(child.get(), target))
            {
                return true;
            }
        }
        return false;
    }

    bool UICanvas::focusAlive()
    {
        if (mFocused == nullptr)
        {
            return false;
        }
        if (this->containsWidget(mRoot.get(), mFocused))
        {
            return true;
        }
        mFocused->triggerBlur();
        mFocused = nullptr;
        return false;
    }
}
