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
        Renderer mRenderer;
        Space mSpace;
        UICanvas mCanvas;

    public:
        UICanvas& canvas() { return mCanvas; }

        void init(HWND windowHandle, int32_t windowWidth, int32_t windowHeight) {
            mRenderer.create(windowHandle, windowWidth, windowHeight);
            mCanvas.init((uint32_t)windowWidth, (uint32_t)windowHeight);

            UINT dpi = GetDpiForWindow(windowHandle);
            if (dpi == 0)
            {
                dpi = 96;
            }
            const float uiScale = (float)dpi / 96.0f;
            const float fontPx = 16.0f * uiScale;

            auto menu = std::make_shared<UIMenu>();
            menu->rect = Rect(0.0f, 0.0f, (float)windowWidth, 32.0f * uiScale);
            menu->direction = UILayoutDirection::Horizontal;
            menu->padding = 4.0f * uiScale;
            menu->spacing = 0.0f;
            menu->color = Color(0.16f, 0.16f, 0.20f, 1.0f);

            const char* fontPath = "modules/test-ui/fonts/Roboto-Regular.ttf";
            auto addItem = [&](const char* title)
            {
                auto item = std::make_shared<UIMenuItem>();
                item->paddingX = 12.0f * uiScale;
                item->paddingY = 6.0f * uiScale;
                item->setText(title);
                if (UIText* label = item->label())
                {
                    label->fontPath = fontPath;
                    label->fontSize = fontPx;
                    label->color = Color(0.92f, 0.92f, 0.94f, 1.0f);
                }
                menu->addItem(item);
            };
            addItem("File");
            addItem("View");
            addItem("About");

            mCanvas.setRoot(menu);
            mCanvas.prepare();

            auto camera = std::make_shared<Camera>();
            camera->viewport.left = 0;
            camera->viewport.top = 0;
            camera->viewport.right = (float)windowWidth;
            camera->viewport.bottom = (float)windowHeight;
            camera->viewport.front = 0.0f;
            camera->viewport.back = 1.0f;
            mSpace.clearColor = Color(0.12f, 0.12f, 0.16f, 1);
            mSpace.add(camera);
            mSpace.activeCamera = camera;
            mSpace.add(mCanvas.shape());
        }

        void quit() {
            mCanvas.quit();
        }

        void tick(float delta) {
            (void)delta;

            mCanvas.flush();
            mRenderer.render(mSpace);
        }
    };
}
