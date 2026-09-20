#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <memory>
#include <vector>

#include "ui/main.h"

namespace eokas::ui {

    inline std::vector<uint8_t> makeSlicedTestPixels(uint32_t size, uint32_t border)
    {
        std::vector<uint8_t> pixels(size * size * 4, 0);
        auto fill = [&](uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t r, uint8_t g, uint8_t b)
        {
            for (uint32_t iy = 0; iy < h; iy++)
            {
                for (uint32_t ix = 0; ix < w; ix++)
                {
                    uint32_t i = ((y + iy) * size + (x + ix)) * 4;
                    pixels[i + 0] = r;
                    pixels[i + 1] = g;
                    pixels[i + 2] = b;
                    pixels[i + 3] = 0xFF;
                }
            }
        };
        uint32_t mid = size - border * 2;
        fill(0, 0, border, border, 220, 60, 60);
        fill(border, 0, mid, border, 230, 200, 50);
        fill(border + mid, 0, border, border, 60, 180, 70);
        fill(0, border, border, mid, 200, 60, 180);
        fill(border, border, mid, mid, 90, 90, 100);
        fill(border + mid, border, border, mid, 50, 180, 200);
        fill(0, border + mid, border, border, 50, 90, 210);
        fill(border, border + mid, mid, border, 230, 130, 40);
        fill(border + mid, border + mid, border, border, 240, 240, 240);
        return pixels;
    }

    class Graphics {
        Device::Ref mDevice;
        UICanvas mCanvas;

    public:
        UICanvas& canvas() { return mCanvas; }

        void init(HWND windowHandle, int32_t windowWidth, int32_t windowHeight) {
            mDevice = GPUFactory::createDevice(windowHandle, windowWidth, windowHeight);
            mCanvas.init(mDevice, (uint32_t)windowWidth, (uint32_t)windowHeight);

            auto layout = std::make_shared<UILayout>();
            layout->rect = Rect(40.0f, 40.0f, 400.0f, 260.0f);
            layout->direction = UILayoutDirection::Vertical;
            layout->padding = 16.0f;
            layout->spacing = 12.0f;
            layout->color = Color(0.2f, 0.2f, 0.25f, 0.0f);

            auto sliced = std::make_shared<UIImage>();
            sliced->rect = Rect(0.0f, 0.0f, 360.0f, 180.0f);
            sliced->uv = Rect(0.0f, 0.0f, 1.0f, 1.0f);
            sliced->border = UIBorder(8.0f, 8.0f, 8.0f, 8.0f);
            sliced->type = UIImageType::Sliced;
            sliced->color = Color(1.0f, 1.0f, 1.0f, 1.0f);
            layout->addChild(sliced);

            mCanvas.setRoot(layout);
            mCanvas.prepare();

            const uint32_t kTestSize = 32;
            const uint32_t kTestBorder = 8;
            TextureOptions options;
            options.width = kTestSize;
            options.height = kTestSize;
            options.mipCount = 1;
            options.format = Format::R8G8B8A8_UNORM;
            Texture::Ref testTexture = mDevice->createTexture(options);
            mCanvas.setTexture(testTexture, makeSlicedTestPixels(kTestSize, kTestBorder));
        }

        void quit() {
            mCanvas.quit();
        }

        void tick(float delta) {
            (void)delta;

            mDevice->waitForGPU();
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
