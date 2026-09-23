#include "UICanvas.h"

#include <cstddef>
#include <map>
#include <stdexcept>
#include <vector>

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
        for (auto& font : mFonts)
        {
            if (font)
            {
                font->close();
            }
        }
        mFonts.clear();
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

    void UICanvas::prepare()
    {
        for (auto& font : mFonts)
        {
            if (font)
            {
                font->close();
            }
        }
        mFonts.clear();

        std::vector<UIText*> texts;
        this->collectTexts(mRoot.get(), texts);

        std::map<String, std::vector<UIText*>> groups;
        for (UIText* text : texts)
        {
            if (text == nullptr || text->fontPath.isEmpty())
            {
                continue;
            }
            groups[text->fontPath].push_back(text);
        }

        for (auto& entry : groups)
        {
            uint32_t pixelSize = (uint32_t)UIText::kDefaultFontSize;
            for (UIText* text : entry.second)
            {
                if (text->fontSize > (float)pixelSize)
                {
                    pixelSize = (uint32_t)text->fontSize;
                }
            }

            UIFont* font = this->loadFont(entry.first.cstr(), pixelSize);
            for (UIText* text : entry.second)
            {
                text->font = font;
            }
        }

        if (mShape && !mFonts.empty() && !mShape->texture)
        {
            UIFont* font = mFonts.front().get();
            mShape->setPendingUpload(font->atlasRgba(), font->atlasSize());
        }
    }

    UIFont* UICanvas::font()
    {
        if (mFonts.empty())
        {
            return nullptr;
        }
        return mFonts.front().get();
    }

    void UICanvas::setTexture(Texture::Ref texture, const std::vector<uint8_t>& rgba)
    {
        if (!mShape || !texture)
        {
            return;
        }

        mShape->setTexture(texture);
        mShape->setPendingUpload(rgba);
        if (mShape->material)
        {
            mShape->material->setParameter(kUIMainTexture, texture);
        }
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
            mRoot->render(*mShape);
        }
        for (auto& font : mFonts)
        {
            if (font && font->takeAtlasDirty())
            {
                mShape->setPendingUpload(font->atlasRgba(), font->atlasSize());
            }
        }
        mShape->end();
    }

    UIShape::Ref UICanvas::shape() const
    {
        return mShape;
    }

    UIWidget* UICanvas::hitTest(float x, float y)
    {
        return this->hitTestNode(mRoot.get(), x, y);
    }

    void UICanvas::onMouseMove(float x, float y)
    {
        if (mPressed != nullptr)
        {
            mPressed->triggerPointerDrag(x, y, mPressedButton);
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
            if (mPressed->acceptsKeyFocus())
            {
                this->setFocus(mPressed);
            }
            else
            {
                this->setFocus(nullptr);
            }
        }
        mPressed->triggerPointerPress();
        mPressed->triggerPointerDrag(x, y, button);
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

    void UICanvas::setFallbackFontPath(const char* path)
    {
        mFallbackFontPath = path != nullptr ? path : "";
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

    UIFont* UICanvas::loadFont(const char* fontPath, uint32_t pixelSize)
    {
        String path = this->resolveAssetPath(fontPath);
        auto font = std::make_unique<UIFont>();
        if (!font->open(path.cstr(), pixelSize))
        {
            throw std::runtime_error("Failed to load UI font.");
        }

        if (!mFallbackFontPath.isEmpty())
        {
            String fallback = this->resolveAssetPath(mFallbackFontPath.cstr());
            font->attachFallback(fallback.cstr());
        }

        UIFont* ptr = font.get();
        mFonts.push_back(std::move(font));
        return ptr;
    }

    void UICanvas::collectTexts(UIWidget* widget, std::vector<UIText*>& texts)
    {
        if (widget == nullptr)
        {
            return;
        }

        UIText* text = dynamic_cast<UIText*>(widget);
        if (text != nullptr)
        {
            texts.push_back(text);
        }

        for (auto& child : widget->children)
        {
            this->collectTexts(child.get(), texts);
        }
    }

    String UICanvas::resolveAssetPath(const char* relativePath) const
    {
        String given = relativePath;
        if (File::exists(given))
        {
            return given;
        }

        String exeDir = File::basePath(Process::executingPath());
        String fromExe = File::combinePath(exeDir, relativePath);
        if (File::exists(fromExe))
        {
            return fromExe;
        }

        String fromBuildDir = File::combinePath(File::basePath(exeDir), relativePath);
        if (File::exists(fromBuildDir))
        {
            return fromBuildDir;
        }

        return given;
    }

    UIWidget* UICanvas::hitTestNode(UIWidget* widget, float x, float y)
    {
        if (widget == nullptr || !widget->visible)
        {
            return nullptr;
        }

        for (auto it = widget->children.rbegin(); it != widget->children.rend(); ++it)
        {
            if (*it)
            {
                UIWidget* hit = this->hitTestNode(it->get(), x, y);
                if (hit != nullptr)
                {
                    return hit;
                }
            }
        }

        if (widget->interactive && widget->rect.contains(Vector2(x, y)))
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
