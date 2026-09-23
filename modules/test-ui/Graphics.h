#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstring>
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
            const char* fallbacks[] = {
                "C:/Windows/Fonts/msyh.ttc",
                "C:/Windows/Fonts/msyh.ttf",
                "C:/Windows/Fonts/simsun.ttc",
                "C:/Windows/Fonts/simhei.ttf",
            };
            for (const char* path : fallbacks)
            {
                if (File::exists(path))
                {
                    mCanvas.setFallbackFontPath(path);
                    break;
                }
            }

            UINT dpi = GetDpiForWindow(windowHandle);
            if (dpi == 0)
            {
                dpi = 96;
            }
            const float uiScale = (float)dpi / 96.0f;
            const float fontPx = 16.0f * uiScale;

            auto root = std::make_shared<UIWidget>();
            root->rect = Rect(0.0f, 0.0f, (float)windowWidth, (float)windowHeight);
            root->interactive = false;

            auto menu = std::make_shared<UIMenu>();
            menu->rect = Rect(0.0f, 0.0f, (float)windowWidth, 0.0f);
            menu->direction = UILayoutDirection::Horizontal;
            menu->padding = 2.0f * uiScale;
            menu->spacing = 0.0f;
            menu->color = Color(0.16f, 0.16f, 0.20f, 1.0f);

            const char* fontPath = "modules/test-ui/fonts/Roboto-Regular.ttf";
            auto addItem = [&](UIMenu& target, const char* title) -> std::shared_ptr<UIMenuItem>
            {
                auto item = std::make_shared<UIMenuItem>();
                item->paddingX = 12.0f * uiScale;
                item->paddingY = 2.0f * uiScale;
                item->background = Color(0.16f, 0.16f, 0.20f, 1.0f);
                item->hoverColor = Color(0.28f, 0.38f, 0.58f, 1.0f);
                item->setText(title);
                if (UIText* label = item->label())
                {
                    label->fontPath = fontPath;
                    label->fontSize = fontPx;
                    label->color = Color(0.92f, 0.92f, 0.94f, 1.0f);
                }
                target.addItem(item);
                return item;
            };
            auto fileItem = addItem(*menu, "File");
            auto viewItem = addItem(*menu, "View");
            auto aboutItem = addItem(*menu, "About");

            auto fileMenu = std::make_shared<UIMenu>();
            fileMenu->direction = UILayoutDirection::Vertical;
            fileMenu->padding = 2.0f * uiScale;
            fileMenu->spacing = 0.0f;
            fileMenu->color = Color(0.16f, 0.16f, 0.20f, 1.0f);
            fileMenu->visible = false;
            addItem(*fileMenu, "Open");
            addItem(*fileMenu, "New");
            addItem(*fileMenu, "Save");

            fileItem->onClick = [fileItem, fileMenu]()
            {
                fileMenu->visible = !fileMenu->visible;
                fileMenu->rect.x = fileItem->rect.x;
                fileMenu->rect.y = fileItem->rect.y + fileItem->rect.height;
            };
            auto hideFileMenu = [fileMenu]() { fileMenu->visible = false; };
            viewItem->onClick = hideFileMenu;
            aboutItem->onClick = hideFileMenu;

            root->children.push_back(menu);
            root->children.push_back(fileMenu);

            auto addButton = [&](const char* title, float x, float y, float w, float h)
            {
                auto button = std::make_shared<UIButton>();
                button->rect = Rect(x, y, w, h);
                button->paddingX = 12.0f * uiScale;
                button->paddingY = 8.0f * uiScale;
                button->setText(title);
                if (UIText* label = button->label())
                {
                    label->fontPath = fontPath;
                    label->fontSize = fontPx;
                    label->color = Color(0.92f, 0.92f, 0.94f, 1.0f);
                }
                String baseTitle = title;
                UIButton* self = button.get();
                button->onClick = [self, baseTitle]()
                {
                    UIText* label = self->label();
                    if (label == nullptr)
                    {
                        return;
                    }
                    String marked = baseTitle + String(" *");
                    label->text = (label->text == marked) ? baseTitle : marked;
                };
                root->children.push_back(button);
            };
            float buttonX = 16.0f * uiScale;
            float buttonY = 48.0f * uiScale;
            float buttonGap = 12.0f * uiScale;
            float okW = 120.0f * uiScale;
            float cancelW = 88.0f * uiScale;
            addButton("OK", buttonX, buttonY, okW, 36.0f * uiScale);
            buttonX += okW + buttonGap;
            addButton("Cancel", buttonX, buttonY, cancelW, 36.0f * uiScale);
            buttonX += cancelW + buttonGap;
            addButton("Apply", buttonX, buttonY, 160.0f * uiScale, 48.0f * uiScale);

            auto addSlider = [&](SliderType type, float x, float y, float w, float h, float labelX, float labelY)
            {
                auto slider = std::make_shared<UISlider>();
                slider->type = type;
                slider->rect = Rect(x, y, w, h);
                slider->minValue = 0.0f;
                slider->maxValue = 100.0f;
                slider->value = 0.0f;
                slider->trackThickness = 2.0f * uiScale;
                slider->thumbSize = 16.0f * uiScale;

                auto label = std::make_shared<UIText>();
                label->rect = Rect(labelX, labelY, 72.0f * uiScale, fontPx);
                label->fontPath = fontPath;
                label->fontSize = fontPx;
                label->color = Color(0.92f, 0.92f, 0.94f, 1.0f);
                label->text = "0";
                UIText* labelPtr = label.get();
                slider->onValueChanged = [labelPtr](float v)
                {
                    labelPtr->text = String::format("%.0f", v);
                };
                root->children.push_back(slider);
                root->children.push_back(label);
            };
            float sliderX = 16.0f * uiScale;
            float sliderY = buttonY + 48.0f * uiScale + 24.0f * uiScale;
            float rowGap = 16.0f * uiScale;
            addSlider(SliderType::Horizontal, sliderX, sliderY, 320.0f * uiScale, 28.0f * uiScale, sliderX + 336.0f * uiScale, sliderY + 4.0f * uiScale);
            sliderY += 28.0f * uiScale + rowGap;
            addSlider(SliderType::Vertical, sliderX, sliderY, 28.0f * uiScale, 140.0f * uiScale, sliderX + 44.0f * uiScale, sliderY);
            addSlider(SliderType::Clock, sliderX + 120.0f * uiScale, sliderY, 140.0f * uiScale, 140.0f * uiScale, sliderX + 268.0f * uiScale, sliderY);
            addSlider(SliderType::Angular, sliderX + 360.0f * uiScale, sliderY, 140.0f * uiScale, 140.0f * uiScale, sliderX + 508.0f * uiScale, sliderY);

            auto addToggle = [&](bool initial, float x, float y)
            {
                float toggleW = 48.0f * uiScale;
                float toggleH = 28.0f * uiScale;
                auto toggle = std::make_shared<UIToggle>();
                toggle->rect = Rect(x, y, toggleW, toggleH);
                toggle->value = initial;

                auto label = std::make_shared<UIText>();
                label->rect = Rect(x + toggleW + 12.0f * uiScale, y + (toggleH - fontPx) * 0.5f, 72.0f * uiScale, fontPx);
                label->fontPath = fontPath;
                label->fontSize = fontPx;
                label->color = Color(0.92f, 0.92f, 0.94f, 1.0f);
                label->text = initial ? "On" : "Off";
                UIText* labelPtr = label.get();
                toggle->onValueChanged = [labelPtr](bool v)
                {
                    labelPtr->text = v ? "On" : "Off";
                };
                root->children.push_back(toggle);
                root->children.push_back(label);
            };
            float toggleY = sliderY + 140.0f * uiScale + rowGap;
            addToggle(false, sliderX, toggleY);
            addToggle(true, sliderX + 140.0f * uiScale, toggleY);

            float inputY = toggleY + 28.0f * uiScale + rowGap;
            std::function<void(const String&)> writeClipboard = [](const String& value)
            {
                if (!OpenClipboard(nullptr))
                {
                    return;
                }
                EmptyClipboard();
                WCString wide = String::utf8ToUnicode(MBString(value.cstr(), value.length()), false);
                size_t bytes = (wide.size() + 1) * sizeof(wchar_t);
                HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, bytes);
                if (mem != nullptr)
                {
                    void* ptr = GlobalLock(mem);
                    if (ptr != nullptr)
                    {
                        memcpy(ptr, wide.c_str(), bytes);
                        GlobalUnlock(mem);
                        SetClipboardData(CF_UNICODETEXT, mem);
                    }
                    else
                    {
                        GlobalFree(mem);
                    }
                }
                CloseClipboard();
            };
            std::function<String()> readClipboard = []() -> String
            {
                String result;
                if (!OpenClipboard(nullptr))
                {
                    return result;
                }
                HANDLE mem = GetClipboardData(CF_UNICODETEXT);
                if (mem != nullptr)
                {
                    const wchar_t* ptr = (const wchar_t*)GlobalLock(mem);
                    if (ptr != nullptr)
                    {
                        MBString utf8 = String::unicodeToUtf8(WCString(ptr), false);
                        result = String(utf8.c_str());
                        GlobalUnlock(mem);
                    }
                }
                CloseClipboard();
                return result;
            };
            auto input = std::make_shared<UIInput>();
            input->rect = Rect(sliderX, inputY, 320.0f * uiScale, 36.0f * uiScale);
            input->paddingX = 8.0f * uiScale;
            input->paddingY = 6.0f * uiScale;
            input->placeholder = "Type and press Enter";
            input->maxLength = 64;
            if (UIText* field = input->label())
            {
                field->fontPath = fontPath;
                field->fontSize = fontPx;
                field->color = Color(0.92f, 0.92f, 0.94f, 1.0f);
            }
            input->writeClipboard = writeClipboard;
            input->readClipboard = readClipboard;
            auto echo = std::make_shared<UIText>();
            echo->rect = Rect(sliderX + 332.0f * uiScale, inputY + (36.0f * uiScale - fontPx) * 0.5f, 240.0f * uiScale, fontPx);
            echo->fontPath = fontPath;
            echo->fontSize = fontPx;
            echo->color = Color(0.92f, 0.92f, 0.94f, 1.0f);
            echo->text = "";
            UIText* echoPtr = echo.get();
            input->onSubmit = [echoPtr](const String& value)
            {
                echoPtr->text = value;
            };
            root->children.push_back(input);
            root->children.push_back(echo);

            float areaY = inputY + 36.0f * uiScale + rowGap;
            auto area = std::make_shared<UIInput>();
            area->multiline = true;
            area->rect = Rect(sliderX, areaY, 320.0f * uiScale, 120.0f * uiScale);
            area->paddingX = 8.0f * uiScale;
            area->paddingY = 6.0f * uiScale;
            area->placeholder = "\xe5\xa4\x9a\xe8\xa1\x8c\xe8\xbe\x93\xe5\x85\xa5\xef\xbc\x8c""Ctrl+Enter \xe6\x8f\x90\xe4\xba\xa4";
            area->writeClipboard = writeClipboard;
            area->readClipboard = readClipboard;
            if (UIText* field = area->label())
            {
                field->fontPath = fontPath;
                field->fontSize = fontPx;
                field->color = Color(0.92f, 0.92f, 0.94f, 1.0f);
            }
            area->setText("\xe7\xac\xac\xe4\xb8\x80\xe8\xa1\x8c\n\xe7\xac\xac\xe4\xba\x8c\xe8\xa1\x8c");
            area->onSubmit = [echoPtr](const String& value)
            {
                echoPtr->text = value.replace("\n", " ");
            };
            root->children.push_back(area);

            mCanvas.setRoot(root);
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
