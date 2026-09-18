#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <memory>

#include "ui/main.h"

namespace eokas::ui {

    class Graphics {
        Device::Ref mDevice;
        UICanvas mCanvas;

    public:
        UICanvas& canvas() { return mCanvas; }

        void init(HWND windowHandle, int32_t windowWidth, int32_t windowHeight) {
            mDevice = GPUFactory::createDevice(windowHandle, windowWidth, windowHeight);
            mCanvas.init(mDevice, (uint32_t)windowWidth, (uint32_t)windowHeight);

            auto layout = std::make_shared<UILayout>();
            layout->rect = Rect(40.0f, 40.0f, 400.0f, 360.0f);
            layout->direction = UILayoutDirection::Vertical;
            layout->padding = 16.0f;
            layout->spacing = 12.0f;
            layout->color = Color(0.2f, 0.2f, 0.25f, 0.8f);

            auto image = std::make_shared<UIImage>();
            image->rect = Rect(0.0f, 0.0f, 240.0f, 240.0f);
            image->uv = Rect(0.0f, 0.0f, 1.0f, 1.0f);
            image->color = Color(1.0f, 1.0f, 1.0f, 1.0f);
            layout->addChild(image);

            auto text = std::make_shared<UIText>();
            text->rect = Rect(0.0f, 0.0f, 360.0f, 32.0f);
            text->text = "Hello, Eokas UI \x7F";
            text->fontSize = 24.0f;
            text->fontPath = "../modules/test-ui/fonts/Roboto-Regular.ttf";
            text->color = Color(1.0f, 0.92f, 0.4f, 1.0f);
            layout->addChild(text);

            mCanvas.setRoot(layout);
            mCanvas.prepare();
        }

        void quit() {
            mCanvas.quit();
        }

        void tick(float delta) {
            (void)delta;

            mCanvas.beginFrame();
            mCanvas.renderFrame();
            mCanvas.endFrame();
            CommandBuffer::Ref cmd = mCanvas.commandBuffer();
            mDevice->commitCommandBuffer(cmd);
            mDevice->present();
            mDevice->waitForNextFrame();
        }
    };
}
