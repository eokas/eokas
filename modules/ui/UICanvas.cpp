#include "UICanvas.h"

#include <map>
#include <stdexcept>
#include <vector>

namespace eokas
{
    void UICanvas::init(uint32_t width, uint32_t height)
    {
        mWidth = (float)width;
        mHeight = (float)height;
        mHovered = nullptr;
        mPressed = nullptr;
        mPressedButton = -1;

        if (!mRoot)
        {
            mRoot = std::make_shared<UIWidget>();
        }

        mShape = std::make_shared<UIShape>();
        mShape->material = std::make_shared<Material>();
        mShape->material->setShaderPath("../shaders/UI.hlsl");
        mShape->material->addVertexElement({"POSITION", 0, 0, Format::R32G32_FLOAT});
        mShape->material->addVertexElement({"TEXCOORD", 0, 8, Format::R32G32_FLOAT});
        mShape->material->addVertexElement({"COLOR", 0, 16, Format::R32G32B32A32_FLOAT});
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
    }

    void UICanvas::quit()
    {
        this->clearHovered(mRoot.get());
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
        this->clearHovered(mRoot.get());
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
            uint32_t pixelSize = 16;
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
            mShape->material->setParameter("gMainTexture", texture);
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
        this->clearHovered(mRoot.get());
        mHovered = this->hitTest(x, y);
    }

    void UICanvas::onMouseDown(float x, float y, int button)
    {
        this->onMouseMove(x, y);
        mPressed = mHovered;
        mPressedButton = button;
    }

    void UICanvas::onMouseUp(float x, float y, int button)
    {
        if (mPressed != nullptr && button == mPressedButton)
        {
            mPressed = nullptr;
            mPressedButton = -1;
        }
        this->onMouseMove(x, y);
    }

    UIFont* UICanvas::loadFont(const char* fontPath, uint32_t pixelSize)
    {
        String path = this->resolveAssetPath(fontPath);
        auto font = std::make_unique<UIFont>();
        if (!font->open(path.cstr(), pixelSize))
        {
            throw std::runtime_error("Failed to load UI font.");
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
                    widget->hovered = true;
                    return hit;
                }
            }
        }

        if (widget->rect.contains(Vector2(x, y)))
        {
            widget->hovered = true;
            return widget;
        }
        return nullptr;
    }

    void UICanvas::clearHovered(UIWidget* widget)
    {
        if (widget == nullptr)
        {
            return;
        }
        widget->hovered = false;
        for (auto& child : widget->children)
        {
            this->clearHovered(child.get());
        }
    }
}
