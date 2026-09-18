#ifndef _EOKAS_UI_CANVAS_H_
#define _EOKAS_UI_CANVAS_H_

#include "UIWidget.h"
#include "UIShape.h"
#include "UIFont.h"
#include "UIText.h"
#include <memory>
#include <vector>

namespace eokas
{
    class UICanvas
    {
    public:
        void init(Device::Ref device, uint32_t width, uint32_t height);
        void quit();

        Device::Ref device() const;
        CommandBuffer::Ref commandBuffer() const;

        const std::shared_ptr<UIWidget>& root() const;
        void setRoot(const std::shared_ptr<UIWidget>& widget);

        void prepare();
        UIFont* font();

        void beginFrame();
        void renderFrame();
        void endFrame();
        UIShape& shape();

        UIWidget* hitTest(float x, float y);
        void onMouseMove(float x, float y);
        void onMouseDown(float x, float y, int button);
        void onMouseUp(float x, float y, int button);

    private:
        Program::Ref compileShader(const char* file, ProgramType type, ProgramTarget target, const char* entry);
        String resolveAssetPath(const char* relativePath) const;
        void collectTexts(UIWidget* widget, std::vector<UIText*>& texts);
        UIFont* loadFont(const char* fontPath, uint32_t pixelSize);
        void bindFontAtlas(UIFont* font);
        void uploadShape();
        UIWidget* hitTestNode(UIWidget* widget, float x, float y);

        float mWidth = 0.0f;
        float mHeight = 0.0f;

        Device::Ref mDevice;
        PipelineObject::Ref mPipelineObject;
        PipelineBindings::Ref mPipelineBindings;
        CommandBuffer::Ref mCommandBuffer;
        Texture::Ref mTexture;

        std::shared_ptr<UIWidget> mRoot;

        std::vector<std::unique_ptr<UIFont>> mFonts;

        UIShape mShape;

        UIWidget* mHovered = nullptr;
        UIWidget* mPressed = nullptr;
        int mPressedButton = -1;
    };
}

#endif//_EOKAS_UI_CANVAS_H_
