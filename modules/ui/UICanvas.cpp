#include "UICanvas.h"

#include <map>
#include <stdexcept>
#include <vector>

namespace eokas
{
    void UICanvas::init(Device::Ref device, uint32_t width, uint32_t height)
    {
        mDevice = device;
        mWidth = (float)width;
        mHeight = (float)height;
        mHovered = nullptr;
        mPressed = nullptr;
        mPressedButton = -1;

        if (!mRoot)
        {
            mRoot = std::make_shared<UIWidget>();
        }

        mShape.create(mDevice);

        const char* hlsl = "../shaders/004-ui.hlsl";
        auto vs = this->compileShader(hlsl, ProgramType::Vertex, ProgramTarget::SM_5_0, "VSMain");
        auto ps = this->compileShader(hlsl, ProgramType::Fragment, ProgramTarget::SM_5_0, "PSMain");

        std::vector<VertexElement> vElements;
        vElements.push_back({"POSITION", 0, 0, Format::R32G32_FLOAT});
        vElements.push_back({"TEXCOORD", 0, 8, Format::R32G32_FLOAT});
        vElements.push_back({"COLOR", 0, 16, Format::R32G32B32A32_FLOAT});

        mPipelineObject = mDevice->createPipelineObject();
        mPipelineObject->begin();
        mPipelineObject->setProgram(ProgramType::Vertex, vs);
        mPipelineObject->setProgram(ProgramType::Fragment, ps);
        mPipelineObject->setVertexElements(vElements);
        mPipelineObject->setCullMode(CullMode::None);
        DepthStencilState depthStencil;
        depthStencil.depthTest = false;
        depthStencil.depthWrite = false;
        depthStencil.depthFunc = CompareOp::Always;
        mPipelineObject->setDepthStencilState(depthStencil);
        SamplerState sampler;
        sampler.minFilter = SamplerFilterMode::Linear;
        sampler.magFilter = SamplerFilterMode::Linear;
        sampler.mipFilter = SamplerFilterMode::Point;
        sampler.addressU = SamplerAddressMode::Clamp;
        sampler.addressV = SamplerAddressMode::Clamp;
        sampler.addressW = SamplerAddressMode::Clamp;
        mPipelineObject->setSamplerState(0, sampler);
        BlendState blend;
        blend.enabled = true;
        blend.srcColor = BlendFactor::SrcAlpha;
        blend.dstColor = BlendFactor::OneMinusSrcAlpha;
        blend.colorOp = BlendOp::Add;
        blend.srcAlpha = BlendFactor::One;
        blend.dstAlpha = BlendFactor::OneMinusSrcAlpha;
        blend.alphaOp = BlendOp::Add;
        mPipelineObject->setBlendState(blend);
        mPipelineObject->end();

        mPipelineBindings = mDevice->createPipelineBindings(mPipelineObject);
        mPipelineBindings->begin();
        mPipelineBindings->setUniformBufferByName("Transform", mShape.uniformBuffer);
        mCommandBuffer = mDevice->createCommandBuffer(mPipelineBindings);
    }

    void UICanvas::quit()
    {
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
        mTexture.reset();
        mCommandBuffer.reset();
        mPipelineBindings.reset();
        mPipelineObject.reset();
        mDevice.reset();
    }

    Device::Ref UICanvas::device() const
    {
        return mDevice;
    }

    CommandBuffer::Ref UICanvas::commandBuffer() const
    {
        return mCommandBuffer;
    }

    const std::shared_ptr<UIWidget>& UICanvas::root() const
    {
        return mRoot;
    }

    void UICanvas::setRoot(const std::shared_ptr<UIWidget>& widget)
    {
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

        std::map<std::string, std::vector<UIText*>> groups;
        for (UIText* text : texts)
        {
            if (text == nullptr || text->fontPath.isEmpty())
            {
                continue;
            }
            groups[text->fontPath.cstr()].push_back(text);
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

            UIFont* font = this->loadFont(entry.first.c_str(), pixelSize);
            for (UIText* text : entry.second)
            {
                text->font = font;
            }
        }

        if (!mFonts.empty())
        {
            this->bindFontAtlas(mFonts.front().get());
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

    void UICanvas::bindFontAtlas(UIFont* font)
    {
        TextureOptions options;
        options.width = font->atlasSize();
        options.height = font->atlasSize();
        options.mipCount = 1;
        options.format = Format::R8G8B8A8_UNORM;
        mTexture = mDevice->createTexture(options);

        mCommandBuffer->fillTexture(mTexture, font->atlasRgba());
        mCommandBuffer->finish();
        mDevice->commitCommandBuffer(mCommandBuffer);
        mDevice->waitForGPU();

        mShape.setTexture(mTexture);
        mPipelineBindings->setTextureByName("gMainTexture", mTexture);
        mPipelineBindings->end();
    }

    void UICanvas::uploadShape()
    {
        Matrix4 proj = Matrix4::IDENTITY;
        proj.value[0][0] = 2.0f / mWidth;
        proj.value[1][1] = -2.0f / mHeight;
        proj.value[3][0] = -1.0f;
        proj.value[3][1] = 1.0f;
        mShape.setProjection(proj);

        mShape.begin();
        if (mRoot)
        {
            mRoot->render(mShape);
        }
        mShape.end();
    }

    void UICanvas::beginFrame()
    {
        mCommandBuffer->reset(mPipelineBindings);

        Viewport viewport;
        viewport.left = 0;
        viewport.top = 0;
        viewport.right = mWidth;
        viewport.bottom = mHeight;
        viewport.front = 0.0f;
        viewport.back = 1.0f;
        mCommandBuffer->setViewport(viewport);

        RenderTarget::Ref renderTarget = mDevice->getActiveRenderTarget();
        RenderTarget::Ref depthTarget = mDevice->getActiveDepthTarget();

        Barrier begin;
        begin.resource = renderTarget;
        begin.before = ResourceState::Present;
        begin.after = ResourceState::RenderTarget;
        mCommandBuffer->barrier({begin});

        mCommandBuffer->setRenderTargets({renderTarget}, depthTarget);
        float clearColor[4] = {0.12f, 0.12f, 0.16f, 1.0f};
        mCommandBuffer->clearRenderTarget(renderTarget, clearColor);
        mCommandBuffer->clearDepthStencil(depthTarget);
        mCommandBuffer->setTopology(Topology::TriangleList);
    }

    void UICanvas::renderFrame()
    {
        this->uploadShape();
        mCommandBuffer->setVertexBuffer(mShape.vertexBuffer, mShape.vertexLength, mShape.vertexStride);
        mCommandBuffer->setIndexBuffer(mShape.indexBuffer, mShape.indexLength, mShape.indexFormat);
        mCommandBuffer->drawIndexedInstanced(mShape.indexCount, 1, 0, 0, 0);
    }

    void UICanvas::endFrame()
    {
        RenderTarget::Ref renderTarget = mDevice->getActiveRenderTarget();

        Barrier end;
        end.resource = renderTarget;
        end.before = ResourceState::RenderTarget;
        end.after = ResourceState::Present;
        mCommandBuffer->barrier({end});

        mCommandBuffer->finish();
    }

    UIShape& UICanvas::shape()
    {
        return mShape;
    }

    UIWidget* UICanvas::hitTest(float x, float y)
    {
        return this->hitTestNode(mRoot.get(), x, y);
    }

    void UICanvas::onMouseMove(float x, float y)
    {
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

    Program::Ref UICanvas::compileShader(const char* file, ProgramType type, ProgramTarget target, const char* entry)
    {
        String path = this->resolveAssetPath(file);
        String source;
        if (!File::readText(path, source))
        {
            throw std::runtime_error("Failed to read UI shader.");
        }

        ProgramOptions options;
        options.name = path.cstr();
        options.source = source.cstr();
        options.type = type;
        options.target = target;
        options.entry = entry;
        return mDevice->createProgram(options);
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

        if (widget->rect.contains(Vector2(x, y)))
        {
            return widget;
        }
        return nullptr;
    }
}
