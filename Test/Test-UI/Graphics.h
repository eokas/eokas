#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cmath>
#include <cstring>
#include <memory>
#include <vector>

#include "Native/NativeWindow.h"
#include "UI/Main.h"
#include "UIHost.h"

namespace eokas::ui {

    template<typename T>
    void setBox(const T& widget, const Rect& box)
    {
        widget->shape.size = box.size;
        widget->shape.origin = box.origin + widget->shape.pivot * widget->shape.size;
    }

    class Graphics {
        Renderer mRenderer;
        Surface::Ref mSurface;
        struct FloatWindow
        {
            NativeWindow native;
            UIWindow* uiWindow = nullptr;
            Surface::Ref surface;
            Space space;
            Camera::Ref camera;
            Rect screenRect;
            bool shapeReady = false;
            bool shown = false;
            bool closing = false;
        };
        NativeWindow mNativeWindow;
        std::vector<std::unique_ptr<FloatWindow>> mFloatWindows;
        std::vector<std::unique_ptr<FloatWindow>> mClosingFloatWindows;
        std::unique_ptr<UIHost> mHost;
        Space mSpace;
        UIWindow* mWindow = nullptr;
        std::shared_ptr<UIMenu> mMenu;
        std::shared_ptr<UIView> mView;
        std::shared_ptr<UIDockSpace> mDock;
        std::shared_ptr<UIList> mSections;
        std::shared_ptr<UIColorPicker> mColorPicker;
        float mClientWidth = 0.0f;
        float mClientHeight = 0.0f;

    public:
        ~Graphics()
        {
            this->quit();
        }

        UIWindow& window() { return *mWindow; }

        bool isOpen() const
        {
            return mNativeWindow.isOpen();
        }

        void pump()
        {
            mNativeWindow.pumpMessages();
            std::vector<FloatWindow*> live;
            live.reserve(mFloatWindows.size());
            for (auto& item : mFloatWindows)
            {
                if (item) live.push_back(item.get());
            }
            for (FloatWindow* item : live)
            {
                item->native.pumpMessages();
            }
            std::vector<FloatWindow*> closing;
            closing.reserve(mClosingFloatWindows.size());
            for (auto& item : mClosingFloatWindows)
            {
                if (item) closing.push_back(item.get());
            }
            for (FloatWindow* item : closing)
            {
                item->native.pumpMessages();
            }
        }

        bool init(float logicalWidth, float logicalHeight) {
            if (logicalWidth < 1.0f) logicalWidth = 1.0f;
            if (logicalHeight < 1.0f) logicalHeight = 1.0f;
            if (!mNativeWindow.create(String("test-ui"), logicalWidth, logicalHeight)) {
                return false;
            }
            HWND windowHandle = (HWND)mNativeWindow.handle();
            UINT dpi = GetDpiForWindow(windowHandle);
            if (dpi == 0)
            {
                dpi = 96;
            }
            const float uiScale = (float)dpi / 96.0f;
            mNativeWindow.setClientSize(logicalWidth * uiScale, logicalHeight * uiScale);
            mNativeWindow.setPosition(80.0f, 40.0f);
            Vector2 client = mNativeWindow.clientSize();
            const float windowWidth = client.x >= 1.0f ? client.x : logicalWidth * uiScale;
            const float windowHeight = client.y >= 1.0f ? client.y : logicalHeight * uiScale;

            mRenderer.create();
            mSurface = mRenderer.attach(windowHandle, (uint32_t)windowWidth, (uint32_t)windowHeight);
            mHost = std::make_unique<UIHost>();
            mHost->onCreateWindow = [this](UIWindow& window, const Rect& screenRect)
            {
                this->createFloatingWindow(window, screenRect);
            };
            mHost->onDestroyWindow = [this](UIWindow& window)
            {
                this->destroyFloatingWindow(window);
            };
            mHost->onPlaceWindow = [this](UIWindow& window, const Rect& screenRect)
            {
                for (auto& item : mFloatWindows)
                {
                    if (item && item->uiWindow == &window) item->screenRect = screenRect;
                }
            };
            mWindow = &mHost->open((uint32_t)windowWidth, (uint32_t)windowHeight);
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
                    mHost->setFallbackFontPath(path);
                    break;
                }
            }

            const float fontPx = 15.0f * uiScale;
            const char* fontPath = "Content/Fonts/RobotoRegular.ttf";

            const float s = uiScale;
            const float titlePx = 22.0f * s;
            const float smallPx = 13.0f * s;
            const float pagePad = 24.0f * s;
            const float colGap = 20.0f * s;
            const float cardPad = 16.0f * s;
            const float cardGap = 16.0f * s;
            const float controlH = 36.0f * s;
            const float buttonH = 34.0f * s;

            const Color canvas(0.102f, 0.110f, 0.133f, 1.0f);
            const Color menuBar(0.078f, 0.086f, 0.110f, 1.0f);
            const Color menuHover(0.196f, 0.275f, 0.412f, 1.0f);
            const Color popup(0.137f, 0.149f, 0.184f, 1.0f);
            const Color cardColor(0.165f, 0.176f, 0.216f, 1.0f);
            const Color ink(0.925f, 0.933f, 0.953f, 1.0f);
            const Color mute(0.580f, 0.620f, 0.690f, 1.0f);
            const Color accent(0.345f, 0.545f, 0.930f, 1.0f);
            const Color accentHover(0.455f, 0.635f, 0.965f, 1.0f);
            const Color accentPress(0.255f, 0.410f, 0.760f, 1.0f);
            const Color neutral(0.227f, 0.247f, 0.306f, 1.0f);
            const Color neutralHover(0.290f, 0.325f, 0.400f, 1.0f);
            const Color neutralPress(0.165f, 0.180f, 0.227f, 1.0f);
            const Color field(0.110f, 0.118f, 0.149f, 1.0f);
            const Color fieldHover(0.149f, 0.165f, 0.208f, 1.0f);
            const Color track(0.227f, 0.247f, 0.310f, 1.0f);
            const Color regionHead(0.216f, 0.239f, 0.310f, 1.0f);
            const Color regionHeadHover(0.275f, 0.345f, 0.490f, 1.0f);
            const Color regionBody(0.133f, 0.145f, 0.180f, 1.0f);
            const Color clear(0.0f, 0.0f, 0.0f, 0.0f);

            auto textOf = [&](const char* value, float width, float height, float size, const Color& color)
            {
                auto label = std::make_shared<UIText>();
                setBox(label, Rect(0.0f, 0.0f, width, height));
                label->maxWidth = width;
                label->style.fontPath = fontPath;
                label->style.fontSize = size;
                label->style.color = color;
                label->text = value;
                return label;
            };
            auto vCenter = [&](const std::shared_ptr<UIWidget>& child, float width, float height)
            {
                auto cell = std::make_shared<UIList>();
                cell->direction = UIDirection::Vertical;
                cell->padding = 0.0f;
                cell->spacing = 0.0f;
                cell->color = clear;
                setBox(cell, Rect(0.0f, 0.0f, width, height));
                float gap = (height - child->shape.size.y) * 0.5f;
                if (gap < 0.0f)
                {
                    gap = 0.0f;
                }
                auto spacer = std::make_shared<UIWidget>();
                spacer->pickable = false;
                setBox(spacer, Rect(0.0f, 0.0f, width, gap));
                cell->attachChild(spacer);
                cell->attachChild(child);
                return cell;
            };
            auto column = [&](float width)
            {
                auto layout = std::make_shared<UIList>();
                layout->direction = UIDirection::Vertical;
                layout->padding = 0.0f;
                layout->spacing = cardGap;
                layout->color = clear;
                setBox(layout, Rect(0.0f, 0.0f, width, 0.0f));
                return layout;
            };
            auto card = [&](float width)
            {
                auto layout = std::make_shared<UIList>();
                layout->direction = UIDirection::Vertical;
                layout->padding = cardPad;
                layout->spacing = 12.0f * s;
                layout->color = cardColor;
                setBox(layout, Rect(0.0f, 0.0f, width, 0.0f));
                return layout;
            };

            auto root = std::make_shared<UIWidget>();
            // Child positions are relative to this pivot. The window root stays at the
            // client top-left so layout and mouse coordinates share one space.
            root->shape.pivot = Vector2(0.0f, 0.0f);
            setBox(root, Rect(0.0f, 0.0f, (float)windowWidth, (float)windowHeight));
            root->pickable = false;

            auto menu = std::make_shared<UIMenu>();
            setBox(menu, Rect(0.0f, 0.0f, (float)windowWidth, 0.0f));
            menu->direction = UIDirection::Horizontal;
            menu->padding = 4.0f * s;
            menu->spacing = 2.0f * s;
            menu->color = menuBar;

            auto status = textOf("Ready", 240.0f * s, smallPx, smallPx, mute);
            UIText* statusPtr = status.get();
            auto note = [statusPtr, ink](const char* message)
            {
                statusPtr->style.color = ink;
                statusPtr->text = message;
            };

            auto addItem = [&](UIMenu& target, const char* title, const Color& background) -> std::shared_ptr<UIMenuItem>
            {
                auto item = std::make_shared<UIMenuItem>();
                item->padding = 8.0f * s;
                item->background = background;
                item->hoverFill = menuHover;
                item->setText(title);
                if (UIText* label = item->label())
                {
                    label->style.fontPath = fontPath;
                    label->style.fontSize = fontPx;
                    label->style.color = ink;
                }
                target.addItem(item);
                return item;
            };
            auto fileItem = addItem(*menu, "File", menuBar);
            auto viewItem = addItem(*menu, "View", menuBar);
            auto aboutItem = addItem(*menu, "About", menuBar);

            auto fileMenu = std::make_shared<UIMenu>();
            fileMenu->direction = UIDirection::Vertical;
            fileMenu->padding = 4.0f * s;
            fileMenu->spacing = 2.0f * s;
            fileMenu->color = popup;
            fileMenu->overlay = true;
            fileMenu->visible = false;
            fileMenu->host = fileItem.get();
            auto newItem = addItem(*fileMenu, "New", popup);
            auto openItem = addItem(*fileMenu, "Open", popup);
            auto saveItem = addItem(*fileMenu, "Save", popup);

            fileItem->onClick = [fileItem, fileMenu, menu]()
            {
                fileMenu->visible = !fileMenu->visible;
                Vector2 itemInList = fileItem->shape.origin - Vector2(
                    fileItem->shape.pivot.x * fileItem->shape.size.x,
                    fileItem->shape.pivot.y * fileItem->shape.size.y);
                Vector2 itemInMenu = menu->list->shape.origin + menu->list->shape.scale * itemInList;
                Vector2 itemInRoot = menu->shape.origin + menu->shape.scale * itemInMenu;
                float itemHeight = fileItem->shape.size.y * menu->list->shape.scale.y * menu->shape.scale.y;
                fileMenu->shape.origin = Vector2(itemInRoot.x, itemInRoot.y + itemHeight) + fileMenu->shape.pivot * fileMenu->shape.size;
            };
            auto command = [&](const std::shared_ptr<UIMenuItem>& item, const char* message)
            {
                item->onClick = [fileMenu, note, message]()
                {
                    fileMenu->visible = false;
                    note(message);
                };
            };
            command(newItem, "New document");
            command(openItem, "Open");
            command(saveItem, "Save");
            viewItem->onClick = [fileMenu, note]()
            {
                fileMenu->visible = false;
                note("View");
            };
            aboutItem->onClick = [fileMenu, note]()
            {
                fileMenu->visible = false;
                note("test-ui");
            };

            root->attachChild(menu);
            root->attachChild(fileMenu);

            auto view = std::make_shared<UIView>();
            setBox(view, Rect(0.0f, 0.0f, (float)windowWidth, (float)windowHeight));
            view->color = canvas;
            view->scrollbarThickness = 8.0f * s;
            view->scrollbar = Color(0.40f, 0.44f, 0.52f, 1.0f);
            view->scrollbarTrack = Color(0.078f, 0.086f, 0.110f, 1.0f);
            view->scrollbarPressed = accent;

            float innerW = (float)windowWidth - pagePad * 2.0f;
            float colW = (float)(int)((innerW - colGap) * 0.5f);
            float cardInner = colW - cardPad * 2.0f;
            status->shape.origin.x += status->shape.pivot.x * (cardInner - status->shape.size.x);
            status->shape.size.x = cardInner;

            auto addButton = [&](const char* title, float width, bool primary)
            {
                auto button = std::make_shared<UIButton>();
                setBox(button, Rect(0.0f, 0.0f, width, buttonH));
                button->paddingX = 12.0f * s;
                button->paddingY = 8.0f * s;
                button->background = primary ? accent : neutral;
                button->hoverFill = primary ? accentHover : neutralHover;
                button->pressedFill = primary ? accentPress : neutralPress;
                button->setText(title);
                if (UIText* label = button->label())
                {
                    label->style.fontPath = fontPath;
                    label->style.fontSize = fontPx;
                    label->style.color = ink;
                }
                String message = title;
                button->onClick = [note, message]()
                {
                    note(message.cstr());
                };
                return button;
            };

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

            auto hrow = [&](float width, float height, float spacing)
            {
                auto row = std::make_shared<UIList>();
                row->direction = UIDirection::Horizontal;
                row->padding = 0.0f;
                row->spacing = spacing;
                row->color = clear;
                setBox(row, Rect(0.0f, 0.0f, width, height));
                return row;
            };
            auto styleField = [&](UIInput& input)
            {
                input.paddingX = 12.0f * s;
                input.paddingY = 8.0f * s;
                input.border.thickness = s;
                input.background = field;
                input.hoverFill = fieldHover;
                input.border.color = accent;
                input.placeholderStyle.color = mute;
                input.caret.color = ink;
                input.selection = Color(0.275f, 0.400f, 0.680f, 1.0f);
                if (UIText* label = input.label())
                {
                    label->style.fontPath = fontPath;
                    label->style.fontSize = fontPx;
                    label->style.color = ink;
                }
                input.writeClipboard = writeClipboard;
                input.readClipboard = readClipboard;
            };
            auto paintSlider = [&](UISlider& slider, float thickness)
            {
                slider.minValue = 0.0f;
                slider.maxValue = 100.0f;
                slider.trackThickness = thickness;
                slider.thumbSize = 14.0f * s;
                slider.track = track;
                slider.fill = accent;
                slider.thumb = ink;
                slider.thumbHover = accentHover;
                slider.thumbPressed = accentPress;
            };
            auto paintToggle = [&](UIToggle& toggle)
            {
                toggle.off = track;
                toggle.on = accent;
                toggle.offHover = neutralHover;
                toggle.onHover = accentHover;
                toggle.offPressed = neutralPress;
                toggle.onPressed = accentPress;
                toggle.thumb = ink;
            };

            auto actions = card(colW);
            actions->attachChild(textOf("Actions", cardInner, smallPx, smallPx, mute));
            float buttonGap = 10.0f * s;
            float buttonW = (float)(int)((cardInner - buttonGap * 2.0f) / 3.0f);
            auto buttons = hrow(cardInner, buttonH, buttonGap);
            buttons->attachChild(addButton("OK", buttonW, false));
            buttons->attachChild(addButton("Cancel", buttonW, false));
            buttons->attachChild(addButton("Apply", buttonW, true));
            actions->attachChild(buttons);

            float toggleW = 44.0f * s;
            float toggleH = 24.0f * s;
            float toggleGap = 12.0f * s;
            auto addToggle = [&](const char* name, bool initial, const char* onText, const char* offText)
            {
                auto row = hrow(cardInner, toggleH, toggleGap);
                auto label = textOf(name, cardInner - toggleW - toggleGap, fontPx, fontPx, ink);
                auto toggle = std::make_shared<UIToggle>();
                setBox(toggle, Rect(0.0f, 0.0f, toggleW, toggleH));
                toggle->value = initial;
                paintToggle(*toggle);
                toggle->onValueChanged = [note, onText, offText](bool value)
                {
                    note(value ? onText : offText);
                };
                row->attachChild(vCenter(label, label->shape.size.x, toggleH));
                row->attachChild(toggle);
                return row;
            };
            actions->attachChild(addToggle("Notifications", false, "Notifications on", "Notifications off"));
            actions->attachChild(addToggle("Preview", true, "Preview on", "Preview off"));
            actions->attachChild(status);
            actions->refit();

            auto fields = card(colW);
            fields->spacing = 10.0f * s;
            fields->attachChild(textOf("Fields", cardInner, smallPx, smallPx, mute));
            auto input = std::make_shared<UIInput>();
            setBox(input, Rect(0.0f, 0.0f, cardInner, controlH));
            input->placeholder = "Title";
            input->maxLength = 64;
            styleField(*input);
            auto hint = textOf("Press Enter to commit.", cardInner, smallPx, smallPx, mute);
            UIText* hintPtr = hint.get();
            input->onSubmit = [hintPtr, ink](const String& value)
            {
                hintPtr->style.color = ink;
                hintPtr->text = value.isEmpty() ? String("Committed empty.") : value;
            };
            fields->attachChild(input);
            fields->attachChild(hint);

            auto area = std::make_shared<UIInput>();
            area->multiline = true;
            setBox(area, Rect(0.0f, 0.0f, cardInner, 108.0f * s));
            area->placeholder = "\xe5\xa4\x87\xe6\xb3\xa8\xef\xbc\x8c""Ctrl+Enter \xe6\x8f\x90\xe4\xba\xa4";
            styleField(*area);
            area->setText("Keep the baseline.\n\xe7\x95\x99\xe5\x87\xba\xe4\xb8\x80\xe8\xa1\x8c\xe4\xb8\xad\xe6\x96\x87\xe3\x80\x82");
            area->onSubmit = [note](const String& value)
            {
                String line = value.replace("\n", " ");
                note(line.isEmpty() ? "Committed notes." : line.cstr());
            };
            fields->attachChild(area);

            auto dropdown = std::make_shared<UIDropdown>();
            setBox(dropdown, Rect(0.0f, 0.0f, cardInner, controlH));
            dropdown->paddingX = 12.0f * s;
            dropdown->paddingY = 8.0f * s;
            dropdown->border.thickness = s;
            dropdown->placeholder = "Quality";
            dropdown->background = field;
            dropdown->hoverFill = fieldHover;
            dropdown->pressedFill = accentPress;
            dropdown->border.color = Color(0.322f, 0.361f, 0.439f, 1.0f);
            dropdown->text.color = ink;
            dropdown->placeholderStyle.color = mute;
            dropdown->chevron = mute;
            dropdown->popup = popup;
            dropdown->itemHover = menuHover;
            dropdown->itemPressed = accentPress;
            dropdown->itemSelected = accent;
            if (UIText* caption = dropdown->caption())
            {
                caption->style.fontPath = fontPath;
                caption->style.fontSize = fontPx;
            }
            dropdown->addItem(0, "Low");
            dropdown->addItem(1, "Medium");
            dropdown->addItem(2, "High");
            dropdown->setValue(1);
            UIDropdown* dropPtr = dropdown.get();
            dropdown->onValueChanged = [note, dropPtr](int index)
            {
                String label = dropPtr->labelOf(index);
                if (label.isEmpty())
                {
                    note("Quality");
                    return;
                }
                String message = String("Quality: ") + label;
                note(message.cstr());
            };
            fields->attachChild(dropdown);
            fields->refit();

            auto sliders = card(colW);
            sliders->spacing = 16.0f * s;
            sliders->attachChild(textOf("Range", cardInner, smallPx, smallPx, mute));
            float nameW = 72.0f * s;
            float valueW = 48.0f * s;
            float rowGap = 12.0f * s;
            float sliderH = 28.0f * s;
            float sliderW = cardInner - nameW - valueW - rowGap * 2.0f;
            auto levelRow = hrow(cardInner, sliderH, rowGap);
            auto levelName = textOf("Level", nameW, fontPx, fontPx, ink);
            auto level = std::make_shared<UISlider>();
            level->type = SliderType::Horizontal;
            setBox(level, Rect(0.0f, 0.0f, sliderW, sliderH));
            paintSlider(*level, 4.0f * s);
            auto levelValue = textOf("0", valueW, fontPx, fontPx, ink);
            UIText* levelPtr = levelValue.get();
            level->onValueChanged = [levelPtr](float value)
            {
                levelPtr->text = String::format("%.0f", value);
            };
            level->setValue(64.0f);
            levelRow->attachChild(vCenter(levelName, nameW, sliderH));
            levelRow->attachChild(level);
            levelRow->attachChild(vCenter(levelValue, valueW, sliderH));
            sliders->attachChild(levelRow);

            float dialGap = 12.0f * s;
            float dialCol = (float)(int)((cardInner - dialGap * 2.0f) / 3.0f);
            float dialRowH = smallPx + 8.0f * s + dialCol;
            auto dialRow = hrow(cardInner, dialRowH, dialGap);
            auto makeDial = [&](SliderType type, const char* name, float initial)
            {
                auto col = std::make_shared<UIList>();
                col->direction = UIDirection::Vertical;
                col->padding = 0.0f;
                col->spacing = 8.0f * s;
                col->color = clear;
                setBox(col, Rect(0.0f, 0.0f, dialCol, dialRowH));
                auto caption = textOf(name, dialCol, smallPx, smallPx, mute);
                auto slider = std::make_shared<UISlider>();
                slider->type = type;
                setBox(slider, Rect(0.0f, 0.0f, dialCol, dialCol));
                bool ring = type == SliderType::Clock || type == SliderType::Angular;
                paintSlider(*slider, ring ? 6.0f * s : 4.0f * s);
                UIText* captionPtr = caption.get();
                String captionName = name;
                slider->onValueChanged = [captionPtr, captionName](float value)
                {
                    captionPtr->text = captionName + String("  ") + String::format("%.0f", value);
                };
                slider->setValue(initial);
                col->attachChild(caption);
                col->attachChild(slider);
                return col;
            };
            dialRow->attachChild(makeDial(SliderType::Vertical, "Rise", 36.0f));
            dialRow->attachChild(makeDial(SliderType::Clock, "Clock", 20.0f));
            dialRow->attachChild(makeDial(SliderType::Angular, "Turn", 72.0f));
            sliders->attachChild(dialRow);
            sliders->refit();

            auto sections = card(colW);
            sections->spacing = 10.0f * s;
            sections->attachChild(textOf("Sections", cardInner, smallPx, smallPx, mute));
            auto addRegion = [&](const char* title, bool open) -> std::shared_ptr<UIRegion>
            {
                float headH = 34.0f * s;
                float headPad = (headH - fontPx) * 0.5f;
                if (headPad < 0.0f)
                {
                    headPad = 0.0f;
                }
                float bodyPad = 12.0f * s;
                float bodyGap = 8.0f * s;
                float bodyButtonH = 32.0f * s;
                float bodyH = bodyPad * 2.0f + smallPx + bodyGap + bodyButtonH;
                float bodyInner = cardInner - bodyPad * 2.0f;

                auto region = std::make_shared<UIRegion>();
                setBox(region, Rect(0.0f, 0.0f, cardInner, 0.0f));
                region->setSpacing(0.0f);
                region->color = clear;

                auto head = std::make_shared<UIList>();
                head->direction = UIDirection::Horizontal;
                setBox(head, Rect(0.0f, 0.0f, cardInner, headH));
                head->padding = headPad;
                head->spacing = 0.0f;
                head->color = regionHead;
                head->pickable = true;

                String titleText = title;
                auto titleLabel = textOf("", cardInner - headPad * 2.0f, fontPx, fontPx, ink);
                titleLabel->text = String(open ? "- " : "+ ") + titleText;
                head->attachChild(titleLabel);

                auto body = std::make_shared<UIList>();
                body->direction = UIDirection::Vertical;
                setBox(body, Rect(0.0f, 0.0f, cardInner, bodyH));
                body->padding = bodyPad;
                body->spacing = bodyGap;
                body->color = regionBody;
                body->attachChild(textOf("Stays in the flow while open.", bodyInner, smallPx, smallPx, mute));

                auto action = addButton("Mark", 96.0f * s, false);
                action->shape.origin.y += action->shape.pivot.y * (bodyButtonH - action->shape.size.y);
                action->shape.size.y = bodyButtonH;
                String marked = String("Marked ") + titleText;
                action->onClick = [note, marked]()
                {
                    note(marked.cstr());
                };
                body->attachChild(action);

                region->setHead(head);
                region->setBody(body);
                if (!open)
                {
                    region->setExpanded(false);
                }

                UIText* titlePtr = titleLabel.get();
                UIList* headPtr = head.get();
                region->onExpandedChanged = [titlePtr, titleText](bool expanded)
                {
                    titlePtr->text = String(expanded ? "- " : "+ ") + titleText;
                };
                head->onPointerEnter = [headPtr, regionHeadHover](const UIPointerEvent&)
                {
                    headPtr->color = regionHeadHover;
                };
                head->onPointerLeave = [headPtr, regionHead](const UIPointerEvent&)
                {
                    headPtr->color = regionHead;
                };
                return region;
            };
            sections->attachChild(addRegion("Alpha", true));
            sections->attachChild(addRegion("Beta", false));
            sections->attachChild(textOf("Content below follows the open section.", cardInner, smallPx, smallPx, mute));
            sections->refit();
            mSections = sections;

            auto left = column(colW);
            auto right = column(colW);
            left->attachChild(actions);
            left->attachChild(fields);
            right->attachChild(sliders);
            right->attachChild(sections);
            left->refit();
            right->refit();

            auto page = std::make_shared<UIList>();
            page->direction = UIDirection::Vertical;
            page->padding = pagePad;
            page->spacing = 14.0f * s;
            page->color = clear;
            setBox(page, Rect(0.0f, 0.0f, (float)windowWidth, 0.0f));
            page->attachChild(textOf("Controls", innerW, titlePx, titlePx, ink));
            page->attachChild(textOf("Menus, fields, and folding sections.", innerW, smallPx, smallPx, mute));
            float columnsH = left->shape.size.y;
            if (right->shape.size.y > columnsH)
            {
                columnsH = right->shape.size.y;
            }
            auto columns = hrow(innerW, columnsH, colGap);
            columns->attachChild(left);
            columns->attachChild(right);
            page->attachChild(columns);
            page->refit();
            view->addChild(page);

            mWindow->setRoot(root);
            mHost->prepare();

            fileItem->syncSize();
            viewItem->syncSize();
            aboutItem->syncSize();
            float itemH = fileItem->shape.size.y;
            if (viewItem->shape.size.y > itemH)
            {
                itemH = viewItem->shape.size.y;
            }
            if (aboutItem->shape.size.y > itemH)
            {
                itemH = aboutItem->shape.size.y;
            }
            float menuH = menu->padding * 2.0f + itemH;
            menu->shape.origin.y += menu->shape.pivot.y * (menuH - menu->shape.size.y);
            menu->shape.size.y = menuH;
            float contentTop = menuH;
            float contentH = (float)windowHeight - contentTop;
            auto dock = std::make_shared<UIDockSpace>();
            setBox(dock, Rect(0.0f, contentTop, (float)windowWidth, contentH));
            dock->setScreenMapper([windowHandle](const Rect& local)
            {
                POINT point;
                point.x = (LONG)local.origin.x;
                point.y = (LONG)local.origin.y;
                ClientToScreen(windowHandle, &point);
                return Rect((float)point.x, (float)point.y, local.size.x, local.size.y);
            });
            mHost->registerSpace(dock.get());
            auto headOf = [&](const char* title)
            {
                float tabH = (float)(int)(28.0f * s + 0.5f);
                float tabPx = (float)(int)(13.0f * s + 0.5f);
                float tabPad = (tabH - tabPx) * 0.5f;
                if (tabH < 1.0f) tabH = 1.0f;
                if (tabPx < 1.0f) tabPx = 1.0f;
                if (tabPad < 0.0f) tabPad = 0.0f;
                auto head = std::make_shared<UIList>();
                head->direction = UIDirection::Horizontal;
                head->padding = tabPad;
                head->spacing = 0.0f;
                head->centerCross = true;
                setBox(head, Rect(0.0f, 0.0f, 0.0f, tabH));
                head->color = regionHead;
                auto label = std::make_shared<UIText>();
                label->pickable = false;
                label->style.fontPath = fontPath;
                label->style.fontSize = tabPx;
                label->style.color = ink;
                label->text = title;
                label->maxWidth = 0.0f;
                head->attachChild(label);
                return head;
            };
            auto noteBody = [&](const char* value)
            {
                auto body = std::make_shared<UIList>();
                body->direction = UIDirection::Vertical;
                body->padding = cardPad;
                body->color = regionBody;
                body->attachChild(textOf(value, 280.0f * s, smallPx, smallPx, mute));
                return body;
            };
            auto makePage = [&](const char* title, const std::shared_ptr<UIWidget>& body)
            {
                auto page = std::make_shared<UIDockPage>();
                page->setHead(headOf(title));
                page->setBody(body);
                page->color = regionBody;
                page->activeFill = regionHeadHover;
                page->tabMark = accent;
                mHost->observe(page);
                return page;
            };
            const int dataColumns = 8;
            const int dataRows = 20;
            const float headerWidth = 56.0f * s;
            const float columnWidth = 128.0f * s;
            const float rowHeight = 36.0f * s;
            const Color gridColor(0.45f, 0.50f, 0.62f, 1.0f);
            const Color headerFill = regionHead;
            const Color headerHover = regionHeadHover;
            const Color cellFill(0.145f, 0.157f, 0.196f, 1.0f);
            const Color pickedHead(0.275f, 0.400f, 0.680f, 1.0f);
            const Color pickedCell(0.176f, 0.216f, 0.314f, 1.0f);
            const Color pickedCellHover(0.204f, 0.251f, 0.361f, 1.0f);
            const char* samples[4][4] = {
                { "Name", "Dept", "Score", "Note" },
                { "Ada", "UI", "98", "Lead" },
                { "Lin", "Core", "87", "Review" },
                { "Chen", "Tools", "91", "" },
            };
            auto sheetView = std::make_shared<UIView>();
            sheetView->color = canvas;
            sheetView->scrollbarThickness = 8.0f * s;
            sheetView->scrollbar = Color(0.40f, 0.44f, 0.52f, 1.0f);
            sheetView->scrollbarTrack = Color(0.078f, 0.086f, 0.110f, 1.0f);
            sheetView->scrollbarPressed = accent;
            auto sheet = std::make_shared<UITable>();
            sheet->color = regionBody;
            sheet->cellSpacing = 0.0f;
            sheet->cellPadding = 0.0f;
            sheet->border = UITableBorder(1.0f, 1.0f, 0.0f, 0.0f, gridColor);
            sheet->addColumn(headerWidth);
            for (int column = 0; column < dataColumns; ++column)
            {
                sheet->addColumn(columnWidth);
            }
            for (int row = 0; row < dataRows + 1; ++row)
            {
                sheet->addRow(rowHeight);
            }
            struct SheetSlot
            {
                UITableCell* cell = nullptr;
                std::shared_ptr<UIInput> input;
            };
            struct SheetPick
            {
                bool column = false;
                int index = -1;
            };
            const int sheetRows = dataRows + 1;
            const int sheetColumns = dataColumns + 1;
            std::vector<SheetSlot> slots((size_t)sheetRows * (size_t)sheetColumns);
            for (int row = 0; row < sheetRows; ++row)
            {
                for (int column = 0; column < sheetColumns; ++column)
                {
                    bool lastColumn = column == dataColumns;
                    bool lastRow = row == dataRows;
                    bool header = row == 0 || column == 0;
                    float span = column == 0 ? headerWidth : columnWidth;
                    float right = lastColumn ? 1.0f : 0.0f;
                    float bottom = lastRow ? 1.0f : 0.0f;
                    float innerW = span - 1.0f - right;
                    float innerH = rowHeight - 1.0f - bottom;
                    if (innerW < 0.0f)
                    {
                        innerW = 0.0f;
                    }
                    if (innerH < 0.0f)
                    {
                        innerH = 0.0f;
                    }
                    auto input = std::make_shared<UIInput>();
                    setBox(input, Rect(0.0f, 0.0f, innerW, innerH));
                    input->paddingX = 8.0f * s;
                    input->paddingY = 8.0f * s;
                    input->border.thickness = header ? 0.0f : 1.0f;
                    input->background = header ? headerFill : cellFill;
                    input->hoverFill = header ? headerFill : fieldHover;
                    input->border.color = accent;
                    input->placeholderStyle.color = mute;
                    input->caret.color = ink;
                    input->selection = Color(0.275f, 0.400f, 0.680f, 1.0f);
                    input->readOnly = header;
                    input->pickable = !header;
                    input->writeClipboard = writeClipboard;
                    input->readClipboard = readClipboard;
                    if (UIText* label = input->label())
                    {
                        label->style.fontPath = fontPath;
                        label->style.fontSize = smallPx;
                        label->style.color = header ? mute : ink;
                    }
                    if (row == 0 && column > 0)
                    {
                        char title[2] = { (char)('A' + column - 1), '\0' };
                        input->setText(title);
                    }
                    else if (column == 0 && row > 0)
                    {
                        input->setText(String::valueToString(row));
                    }
                    else if (row > 0 && column > 0 && row <= 4 && column <= 4)
                    {
                        input->setText(samples[row - 1][column - 1]);
                    }
                    UITableCell* cell = sheet->cell(row, column);
                    cell->color = header ? headerFill : cellFill;
                    if (lastColumn || lastRow)
                    {
                        cell->setBorder(UITableBorder(1.0f, 1.0f, right, bottom, gridColor));
                    }
                    sheet->setContent(row, column, input);
                    slots[(size_t)(row * sheetColumns + column)].cell = cell;
                    slots[(size_t)(row * sheetColumns + column)].input = input;
                }
            }
            auto pick = std::make_shared<SheetPick>();
            auto paint = [slots, pick, sheetColumns, headerFill, cellFill, fieldHover, pickedHead, pickedCell, pickedCellHover, mute, ink]()
            {
                int count = (int)slots.size();
                for (int i = 0; i < count; ++i)
                {
                    int row = sheetColumns > 0 ? i / sheetColumns : 0;
                    int column = sheetColumns > 0 ? i % sheetColumns : 0;
                    UIInput* input = slots[(size_t)i].input.get();
                    UITableCell* cell = slots[(size_t)i].cell;
                    if (input == nullptr || cell == nullptr)
                    {
                        continue;
                    }
                    bool header = row == 0 || column == 0;
                    bool headOn = pick->index >= 0 && ((pick->column && row == 0 && column == pick->index) || (!pick->column && column == 0 && row == pick->index));
                    bool bodyOn = pick->index >= 0 && row > 0 && column > 0 && ((pick->column && column == pick->index) || (!pick->column && row == pick->index));
                    Color fill = cellFill;
                    Color hover = fieldHover;
                    Color text = ink;
                    if (headOn)
                    {
                        fill = pickedHead;
                        hover = pickedHead;
                        text = ink;
                    }
                    else if (header)
                    {
                        fill = headerFill;
                        hover = headerFill;
                        text = mute;
                    }
                    else if (bodyOn)
                    {
                        fill = pickedCell;
                        hover = pickedCellHover;
                    }
                    input->background = fill;
                    input->hoverFill = hover;
                    cell->color = fill;
                    if (UIText* label = input->label())
                    {
                        label->style.color = header ? text : ink;
                    }
                }
            };
            for (int i = 0; i < (int)slots.size(); ++i)
            {
                int row = i / sheetColumns;
                int column = i % sheetColumns;
                bool columnHead = row == 0 && column > 0;
                bool rowHead = column == 0 && row > 0;
                UITableCell* cell = slots[(size_t)i].cell;
                std::shared_ptr<UIInput> input = slots[(size_t)i].input;
                if (cell == nullptr || !input)
                {
                    continue;
                }
                if (columnHead || rowHead)
                {
                    cell->pickable = true;
                    int index = columnHead ? column : row;
                    cell->onClick = [pick, paint, columnHead, index]()
                    {
                        pick->column = columnHead;
                        pick->index = index;
                        paint();
                    };
                    cell->onPointerEnter = [input, cell, pick, columnHead, index, headerHover](const UIPointerEvent&)
                    {
                        bool on = pick->index >= 0 && pick->column == columnHead && pick->index == index;
                        if (on)
                        {
                            return;
                        }
                        input->background = headerHover;
                        cell->color = headerHover;
                    };
                    cell->onPointerLeave = [paint](const UIPointerEvent&)
                    {
                        paint();
                    };
                }
            }
            float sheetW = headerWidth + columnWidth * (float)dataColumns;
            float sheetH = rowHeight * (float)(dataRows + 1);
            setBox(sheet, Rect(0.0f, 0.0f, sheetW, sheetH));
            sheetView->addChild(sheet);
            auto detailsView = std::make_shared<UIView>();
            detailsView->color = canvas;
            detailsView->scrollbarThickness = 8.0f * s;
            detailsView->scrollbar = Color(0.40f, 0.44f, 0.52f, 1.0f);
            detailsView->scrollbarTrack = Color(0.078f, 0.086f, 0.110f, 1.0f);
            detailsView->scrollbarPressed = accent;
            const float propNameW = 280.0f * s;
            const float propValueW = 220.0f * s;
            const float treeRowH = 32.0f * s;
            const float treeIndent = 18.0f * s;
            const float treePad = 6.0f * s;
            auto tree = std::make_shared<UITable>();
            tree->color = regionBody;
            tree->cellSpacing = 0.0f;
            tree->cellPadding = treePad;
            tree->border = UITableBorder(1.0f, 1.0f, 0.0f, 0.0f, gridColor);
            tree->disclosure = mute;
            tree->addColumn(propNameW);
            tree->addColumn(propValueW);
            tree->setColumnIndent(0, treeIndent);
            auto paintLine = [&](UITableRow* line, bool group)
            {
                Color fill = group ? headerFill : cellFill;
                for (int column = 0; column < 2; ++column)
                {
                    UITableCell* item = line->cell(column);
                    if (item == nullptr)
                    {
                        continue;
                    }
                    item->color = fill;
                    float right = column == 1 ? 1.0f : 0.0f;
                    item->setBorder(UITableBorder(1.0f, 1.0f, right, 0.0f, gridColor));
                }
            };
            auto bindName = [&](UITableRow* line, const char* name, bool group)
            {
                float shift = (float)(line->depth() + 1) * treeIndent;
                float innerW = propNameW - 1.0f - treePad * 2.0f - shift;
                float innerH = treeRowH - 1.0f - treePad * 2.0f;
                if (innerW < 1.0f) innerW = 1.0f;
                if (innerH < 1.0f) innerH = 1.0f;
                line->setContent(0, textOf(name, innerW, innerH, smallPx, group ? mute : ink));
            };
            auto bindValue = [&](UITableRow* line, const char* value)
            {
                float innerW = propValueW - 2.0f - treePad * 2.0f;
                float innerH = treeRowH - 1.0f - treePad * 2.0f;
                if (innerW < 1.0f) innerW = 1.0f;
                if (innerH < 1.0f) innerH = 1.0f;
                auto input = std::make_shared<UIInput>();
                setBox(input, Rect(0.0f, 0.0f, innerW, innerH));
                input->paddingX = 8.0f * s;
                input->paddingY = 4.0f * s;
                input->border.thickness = 0.0f;
                input->background = cellFill;
                input->hoverFill = fieldHover;
                input->border.color = accent;
                input->caret.color = ink;
                input->placeholderStyle.color = mute;
                input->selection = Color(0.275f, 0.400f, 0.680f, 1.0f);
                input->writeClipboard = writeClipboard;
                input->readClipboard = readClipboard;
                if (UIText* label = input->label())
                {
                    label->style.fontPath = fontPath;
                    label->style.fontSize = smallPx;
                    label->style.color = ink;
                }
                input->setText(value);
                line->setContent(1, input);
            };
            auto addLine = [&](UITableRow* parent, const char* name, const char* value, bool group) -> UITableRow*
            {
                UITableRow* line = parent != nullptr ? parent->addRow(treeRowH) : tree->addRow(treeRowH);
                paintLine(line, group);
                bindName(line, name, group);
                if (!group && value != nullptr)
                {
                    bindValue(line, value);
                }
                return line;
            };
            UITableRow* transform = addLine(nullptr, "Transform", nullptr, true);
            UITableRow* position = addLine(transform, "Position", nullptr, true);
            addLine(position, "X", "0", false);
            addLine(position, "Y", "1.5", false);
            addLine(position, "Z", "0", false);
            UITableRow* rotation = addLine(transform, "Rotation", nullptr, true);
            addLine(rotation, "X", "0", false);
            addLine(rotation, "Y", "45", false);
            addLine(rotation, "Z", "0", false);
            rotation->setExpanded(false);
            UITableRow* scale = addLine(transform, "Scale", nullptr, true);
            addLine(scale, "X", "1", false);
            addLine(scale, "Y", "1", false);
            addLine(scale, "Z", "1", false);
            UITableRow* renderer = addLine(nullptr, "Renderer", nullptr, true);
            addLine(renderer, "Visible", "true", false);
            addLine(renderer, "Mesh", "Cube", false);
            addLine(renderer, "Cast Shadows", "On", false);
            tree->refit();
            detailsView->addChild(tree);
            auto board = std::make_shared<UIWidget>();
            board->pickable = false;
            board->color = clear;
            board->shape.pivot = Vector2(0.0f, 0.0f);
            auto canvasNode = [&](const char* title, float x, float y, const Color& fill, const Color& hover, const Color& press, bool drag)
            {
                auto node = std::make_shared<UIButton>();
                setBox(node, Rect(x, y, 148.0f * s, 52.0f * s));
                node->paddingX = 12.0f * s;
                node->paddingY = 10.0f * s;
                node->background = fill;
                node->hoverFill = hover;
                node->pressedFill = press;
                node->draggable = drag;
                node->setText(title);
                if (UIText* label = node->label())
                {
                    label->style.fontPath = fontPath;
                    label->style.fontSize = smallPx;
                    label->style.color = ink;
                }
                return node;
            };
            auto stage = std::make_shared<UICanvas>();
            stage->color = cardColor;
            setBox(stage, Rect(28.0f * s, 52.0f * s, 0.0f, 0.0f));
            stage->attachChild(canvasNode("Drag", 0.0f, 0.0f, accent, accentHover, accentPress, true));
            stage->attachChild(canvasNode("Move", 188.0f * s, 28.0f * s, neutral, neutralHover, neutralPress, true));
            stage->attachChild(canvasNode("Pinned", 24.0f * s, 108.0f * s, field, fieldHover, field, false));
            auto cluster = std::make_shared<UICanvas>();
            cluster->color = regionHead;
            setBox(cluster, Rect(220.0f * s, 132.0f * s, 0.0f, 0.0f));
            cluster->attachChild(canvasNode("Group A", 0.0f, 0.0f, accent, accentHover, accentPress, true));
            cluster->attachChild(canvasNode("Group B", 176.0f * s, 68.0f * s, neutral, neutralHover, neutralPress, true));
            stage->attachChild(cluster);
            auto ellipse = std::make_shared<UIEllipse>();
            setBox(ellipse, Rect(300.0f * s, 8.0f * s, 200.0f * s, 110.0f * s));
            ellipse->background = accent;
            ellipse->hoverFill = accentHover;
            ellipse->pressedFill = accentPress;
            ellipse->stroke.color = Color(1.0f, 1.0f, 1.0f, 1.0f);
            ellipse->stroke.thickness = 2.0f * s;
            ellipse->selected = true;
            stage->attachChild(ellipse);
            auto styleChartLabel = [&](UIChart& chart)
            {
                if (UIText* label = chart.label())
                {
                    label->style.fontPath = fontPath;
                    label->style.fontSize = smallPx;
                    label->style.color = ink;
                }
            };
            auto process = std::make_shared<UIRectangle>();
            setBox(process, Rect(16.0f * s, 250.0f * s, 150.0f * s, 72.0f * s));
            process->background = neutral;
            process->hoverFill = neutralHover;
            process->pressedFill = neutralPress;
            process->border.color = Color(1.0f, 1.0f, 1.0f, 1.0f);
            process->border.thickness = 2.0f * s;
            process->setText("Process");
            styleChartLabel(*process);
            auto decision = std::make_shared<UIDiamond>();
            setBox(decision, Rect(300.0f * s, 340.0f * s, 150.0f * s, 110.0f * s));
            decision->background = accent;
            decision->hoverFill = accentHover;
            decision->pressedFill = accentPress;
            decision->border.color = Color(1.0f, 1.0f, 1.0f, 1.0f);
            decision->border.thickness = 2.0f * s;
            decision->setText("If");
            styleChartLabel(*decision);
            auto link = std::make_shared<UILink>();
            link->kind = UIPathKind::CubicBezier;
            link->setPoints({
                Vector2(166.0f * s, 286.0f * s),
                Vector2(230.0f * s, 286.0f * s),
                Vector2(236.0f * s, 395.0f * s),
                Vector2(300.0f * s, 395.0f * s)
            });
            link->start.target = process.get();
            link->start.anchor = UIAnchor::Right;
            link->end.target = decision.get();
            link->end.anchor = UIAnchor::Left;
            link->end.cap.kind = UIEndpointKind::ArrowFilled;
            link->end.cap.size = 14.0f * s;
            link->line.color = ink;
            link->line.thickness = 2.0f * s;
            stage->attachChild(process);
            stage->attachChild(decision);
            stage->attachChild(link);
            auto canvasHint = textOf("Drag a card, shape, or link. Drag empty canvas to pan. Scroll to zoom.", 720.0f * s, smallPx, smallPx, mute);
            canvasHint->shape.origin = Vector2(16.0f * s, 14.0f * s) + canvasHint->shape.pivot * canvasHint->shape.size;
            board->attachChild(stage);
            board->attachChild(canvasHint);
            setBox(view, Rect(0.0f, 0.0f, (float)windowWidth, contentH));
            auto controls = makePage("Controls", view);
            dock->dockPage(controls, UIDockMode::Fill);
            dock->dockPage(makePage("Table", sheetView), UIDockMode::Fill);
            dock->dockPage(makePage("Outline", noteBody("Drag this tab to dock or float.")), UIDockMode::Fill);
            dock->dockPage(makePage("Details", detailsView), UIDockMode::Fill);
            auto colorView = std::make_shared<UIView>();
            colorView->color = canvas;
            colorView->scrollbarThickness = 8.0f * s;
            colorView->scrollbar = Color(0.40f, 0.44f, 0.52f, 1.0f);
            colorView->scrollbarTrack = Color(0.078f, 0.086f, 0.110f, 1.0f);
            colorView->scrollbarPressed = accent;
            Vector2 colorSize = UIColorPicker::preferredSize(s);
            auto picker = std::make_shared<UIColorPicker>();
            picker->uiScale = s;
            picker->setFont(fontPath, smallPx);
            picker->background = Color(0.120f, 0.129f, 0.165f, 1.0f);
            picker->field = field;
            picker->ink = ink;
            picker->muted = mute;
            picker->accent = accent;
            picker->button = neutral;
            picker->buttonHover = neutralHover;
            picker->buttonPress = neutralPress;
            picker->line = Color(0.050f, 0.055f, 0.070f, 1.0f);
            picker->writeClipboard = writeClipboard;
            picker->readClipboard = readClipboard;

            const float palettePad = 4.0f;
            const float paletteGap = 2.0f;
            const float palettePadPx = floorf(palettePad * s + 0.5f);
            const float paletteGapPx = floorf(paletteGap * s + 0.5f);
            auto paletteBox = [&](float cellPx)
            {
                return Vector2(
                    palettePadPx * 2.0f + cellPx * (float)UIPalette::kColumns + paletteGapPx * (float)(UIPalette::kColumns - 1),
                    palettePadPx * 2.0f + cellPx * (float)UIPalette::kRows + paletteGapPx * (float)(UIPalette::kRows - 1));
            };
            float minInner = 268.0f * s;
            float cellPx = floorf((minInner - palettePadPx * 2.0f - paletteGapPx * (float)(UIPalette::kColumns - 1)) / (float)UIPalette::kColumns);
            if (cellPx < 22.0f) cellPx = 22.0f;
            Vector2 paletteSize = paletteBox(cellPx);
            float colorSideInner = paletteSize.x;
            float colorSideW = colorSideInner + cardPad * 2.0f;
            float pickerW = innerW - colGap - colorSideW;
            if (pickerW < colorSize.x)
            {
                pickerW = colorSize.x;
                float maxSide = innerW - colGap - pickerW;
                float maxInner = maxSide - cardPad * 2.0f;
                float fitted = floorf((maxInner - palettePadPx * 2.0f - paletteGapPx * (float)(UIPalette::kColumns - 1)) / (float)UIPalette::kColumns);
                if (fitted < 16.0f) fitted = 16.0f;
                cellPx = fitted;
                paletteSize = paletteBox(cellPx);
                colorSideInner = paletteSize.x;
                colorSideW = colorSideInner + cardPad * 2.0f;
                pickerW = innerW - colGap - colorSideW;
                if (pickerW < colorSize.x) pickerW = colorSize.x;
            }
            setBox(picker, Rect(0.0f, 0.0f, pickerW, colorSize.y));

            auto palette = std::make_shared<UIPalette>();
            palette->uiScale = s;
            palette->padding = palettePad;
            palette->spacing = paletteGap;
            palette->cellSize = s > 0.0f ? cellPx / s : cellPx;
            palette->background = field;
            palette->accent = accent;
            palette->line = Color(0.050f, 0.055f, 0.070f, 1.0f);
            setBox(palette, Rect(0.0f, 0.0f, paletteSize.x, paletteSize.y));
            palette->setColor(Color(1.0f, 0.0f, 0.0f, 1.0f));

            float swatchH = 48.0f * s;
            auto preview = std::make_shared<UIWidget>();
            preview->pickable = false;
            setBox(preview, Rect(0.0f, 0.0f, colorSideInner, swatchH));
            auto live = textOf("Live", colorSideInner, smallPx, smallPx, ink);
            auto committed = textOf("Committed", colorSideInner, smallPx, smallPx, mute);
            auto formatColor = [](const Color& value)
            {
                return String::format("%.3f  %.3f  %.3f  %.3f", value.r, value.g, value.b, value.a);
            };
            UIWidget* previewPtr = preview.get();
            UIText* livePtr = live.get();
            UIText* committedPtr = committed.get();
            picker->onColorChanged = [previewPtr, livePtr, formatColor](const Color& value)
            {
                previewPtr->color = value;
                livePtr->text = String("Live  ") + formatColor(value);
            };
            picker->onColorCommitted = [committedPtr, formatColor](const Color& value)
            {
                committedPtr->text = String("Committed  ") + formatColor(value);
            };
            picker->onCancelled = [committedPtr]()
            {
                committedPtr->text = "Reverted to the original color.";
            };
            palette->onColorChanged = [previewPtr, livePtr, committedPtr, formatColor](const Color& value)
            {
                previewPtr->color = value;
                livePtr->text = String("Live  ") + formatColor(value);
                committedPtr->text = String("Committed  ") + formatColor(value);
            };
            picker->setColor(Color(0.90f, 0.35f, 0.12f, 0.82f));
            committedPtr->text = String("Committed  ") + formatColor(picker->color());

            auto previewCard = card(colorSideW);
            previewCard->spacing = 8.0f * s;
            previewCard->attachChild(textOf("Preview", colorSideInner, smallPx, smallPx, mute));
            previewCard->attachChild(preview);
            previewCard->attachChild(live);
            previewCard->attachChild(committed);
            previewCard->refit();

            auto paletteCard = card(colorSideW);
            paletteCard->spacing = 8.0f * s;
            paletteCard->attachChild(textOf("Palette", colorSideInner, smallPx, smallPx, mute));
            paletteCard->attachChild(palette);
            paletteCard->refit();

            auto colorSide = column(colorSideW);
            colorSide->spacing = 14.0f * s;
            colorSide->attachChild(previewCard);
            colorSide->attachChild(paletteCard);
            colorSide->refit();

            float colorBlockH = colorSize.y;
            if (colorSide->shape.size.y > colorBlockH) colorBlockH = colorSide->shape.size.y;
            auto colorColumns = hrow(innerW, colorBlockH, colGap);
            colorColumns->attachChild(colorSide);
            colorColumns->attachChild(picker);

            auto colorPage = std::make_shared<UIList>();
            colorPage->direction = UIDirection::Vertical;
            colorPage->padding = pagePad;
            colorPage->spacing = 14.0f * s;
            colorPage->color = clear;
            setBox(colorPage, Rect(0.0f, 0.0f, (float)windowWidth, 0.0f));
            colorPage->attachChild(textOf("Color", innerW, titlePx, titlePx, ink));
            colorPage->attachChild(textOf("Picker and palette share the result panel. Drag onto Themes to save, right-click to remove. Esc cancels the dropper.", innerW, smallPx, smallPx, mute));
            colorPage->attachChild(colorColumns);
            colorPage->refit();
            colorView->addChild(colorPage);
            auto colorTab = makePage("Color", colorView);

            auto dateView = std::make_shared<UIView>();
            dateView->color = canvas;
            dateView->scrollbarThickness = 8.0f * s;
            dateView->scrollbar = Color(0.40f, 0.44f, 0.52f, 1.0f);
            dateView->scrollbarTrack = Color(0.078f, 0.086f, 0.110f, 1.0f);
            dateView->scrollbarPressed = accent;
            auto paintNumber = [&](UINumberPicker& picker)
            {
                picker.uiScale = s;
                picker.setFont(fontPath, smallPx);
                picker.background = Color(0.120f, 0.129f, 0.165f, 1.0f);
                picker.field = field;
                picker.ink = ink;
                picker.muted = mute;
                picker.accent = accent;
                picker.button = neutral;
                picker.buttonHover = neutralHover;
                picker.buttonPress = neutralPress;
                picker.line = Color(0.050f, 0.055f, 0.070f, 1.0f);
            };
            auto paintTime = [&](UITimePicker& picker)
            {
                picker.uiScale = s;
                picker.setFont(fontPath, smallPx);
                picker.background = Color(0.120f, 0.129f, 0.165f, 1.0f);
                picker.field = field;
                picker.ink = ink;
                picker.muted = mute;
                picker.accent = accent;
                picker.button = neutral;
                picker.buttonHover = neutralHover;
                picker.buttonPress = neutralPress;
                picker.line = Color(0.050f, 0.055f, 0.070f, 1.0f);
            };
            auto paintDate = [&](UIDatePicker& picker)
            {
                picker.uiScale = s;
                picker.setFont(fontPath, smallPx);
                picker.background = Color(0.120f, 0.129f, 0.165f, 1.0f);
                picker.field = field;
                picker.ink = ink;
                picker.muted = mute;
                picker.accent = accent;
                picker.button = neutral;
                picker.buttonHover = neutralHover;
                picker.buttonPress = neutralPress;
                picker.line = Color(0.050f, 0.055f, 0.070f, 1.0f);
            };
            auto paintDateTime = [&](UIDateTimePicker& picker)
            {
                picker.uiScale = s;
                picker.setFont(fontPath, smallPx);
                picker.background = Color(0.120f, 0.129f, 0.165f, 1.0f);
                picker.field = field;
                picker.ink = ink;
                picker.muted = mute;
                picker.accent = accent;
                picker.button = neutral;
                picker.buttonHover = neutralHover;
                picker.buttonPress = neutralPress;
                picker.line = Color(0.050f, 0.055f, 0.070f, 1.0f);
            };

            Vector2 dateSize = UIDatePicker::preferredSize(s);
            Vector2 timeSize = UITimePicker::preferredSize(s, true);
            Vector2 dateTimeSize = UIDateTimePicker::preferredSize(s, true);
            Vector2 dateSizeH = UIDatePicker::preferredSize(s, UIDirection::Horizontal);
            Vector2 timeSizeH = UITimePicker::preferredSize(s, true, UIDirection::Horizontal);
            Vector2 dateTimeSizeH = UIDateTimePicker::preferredSize(s, true, UIDirection::Horizontal);
            auto datePicker = std::make_shared<UIDatePicker>();
            paintDate(*datePicker);
            datePicker->setDate(2026, 11, 7);
            setBox(datePicker, Rect(0.0f, 0.0f, dateSize.x, dateSize.y));
            auto timePicker = std::make_shared<UITimePicker>();
            paintTime(*timePicker);
            timePicker->setTime(13, 30, 45);
            setBox(timePicker, Rect(0.0f, 0.0f, timeSize.x, timeSize.y));
            auto dateTimePicker = std::make_shared<UIDateTimePicker>();
            paintDateTime(*dateTimePicker);
            dateTimePicker->setDateTime(2026, 11, 7, 13, 30, 45);
            setBox(dateTimePicker, Rect(0.0f, 0.0f, dateTimeSize.x, dateTimeSize.y));
            auto datePickerH = std::make_shared<UIDatePicker>();
            datePickerH->direction = UIDirection::Horizontal;
            paintDate(*datePickerH);
            datePickerH->setDate(2026, 11, 7);
            setBox(datePickerH, Rect(0.0f, 0.0f, dateSizeH.x, dateSizeH.y));
            auto timePickerH = std::make_shared<UITimePicker>();
            timePickerH->direction = UIDirection::Horizontal;
            paintTime(*timePickerH);
            timePickerH->setTime(13, 30, 45);
            setBox(timePickerH, Rect(0.0f, 0.0f, timeSizeH.x, timeSizeH.y));
            auto dateTimePickerH = std::make_shared<UIDateTimePicker>();
            dateTimePickerH->direction = UIDirection::Horizontal;
            paintDateTime(*dateTimePickerH);
            dateTimePickerH->setDateTime(2026, 11, 7, 13, 30, 45);
            setBox(dateTimePickerH, Rect(0.0f, 0.0f, dateTimeSizeH.x, dateTimeSizeH.y));

            float dateSideW = 280.0f * s;
            float dateSideInner = dateSideW - cardPad * 2.0f;
            auto dateLive = textOf("Live", dateSideInner, smallPx, smallPx, ink);
            auto dateCommitted = textOf("Committed", dateSideInner, smallPx, smallPx, mute);
            auto formatStamp = [](int year, int month, int day, int hour, int minute, int second)
            {
                return String::format("%04d-%02d-%02d  %02d:%02d:%02d", year, month, day, hour, minute, second);
            };
            UIText* dateLivePtr = dateLive.get();
            UIText* dateCommittedPtr = dateCommitted.get();
            UIDatePicker* datePtr = datePicker.get();
            UITimePicker* timePtr = timePicker.get();
            UIDateTimePicker* dateTimePtr = dateTimePicker.get();
            UIDatePicker* datePtrH = datePickerH.get();
            UITimePicker* timePtrH = timePickerH.get();
            UIDateTimePicker* dateTimePtrH = dateTimePickerH.get();
            auto refreshLive = [dateLivePtr, formatStamp](const char* label, int year, int month, int day, int hour, int minute, int second)
            {
                dateLivePtr->text = String(label) + String("   ") + formatStamp(year, month, day, hour, minute, second);
            };
            auto refreshCommitted = [dateCommittedPtr, formatStamp](const char* label, int year, int month, int day, int hour, int minute, int second)
            {
                dateCommittedPtr->text = String(label) + String("   ") + formatStamp(year, month, day, hour, minute, second);
            };
            datePicker->onDateChanged = [refreshLive, datePtr]()
            {
                refreshLive("Date", datePtr->year(), datePtr->month(), datePtr->day(), 0, 0, 0);
            };
            datePicker->onDateCommitted = [refreshCommitted, datePtr]()
            {
                refreshCommitted("Date", datePtr->year(), datePtr->month(), datePtr->day(), 0, 0, 0);
            };
            timePicker->onTimeChanged = [refreshLive, timePtr]()
            {
                refreshLive("Time", 0, 0, 0, timePtr->hour(), timePtr->minute(), timePtr->second());
            };
            timePicker->onTimeCommitted = [refreshCommitted, timePtr]()
            {
                refreshCommitted("Time", 0, 0, 0, timePtr->hour(), timePtr->minute(), timePtr->second());
            };
            dateTimePicker->onDateTimeChanged = [refreshLive, dateTimePtr]()
            {
                refreshLive("DateTime", dateTimePtr->year(), dateTimePtr->month(), dateTimePtr->day(), dateTimePtr->hour(), dateTimePtr->minute(), dateTimePtr->second());
            };
            dateTimePicker->onDateTimeCommitted = [refreshCommitted, dateTimePtr]()
            {
                refreshCommitted("DateTime", dateTimePtr->year(), dateTimePtr->month(), dateTimePtr->day(), dateTimePtr->hour(), dateTimePtr->minute(), dateTimePtr->second());
            };
            datePickerH->onDateChanged = [refreshLive, datePtrH]()
            {
                refreshLive("Date", datePtrH->year(), datePtrH->month(), datePtrH->day(), 0, 0, 0);
            };
            datePickerH->onDateCommitted = [refreshCommitted, datePtrH]()
            {
                refreshCommitted("Date", datePtrH->year(), datePtrH->month(), datePtrH->day(), 0, 0, 0);
            };
            timePickerH->onTimeChanged = [refreshLive, timePtrH]()
            {
                refreshLive("Time", 0, 0, 0, timePtrH->hour(), timePtrH->minute(), timePtrH->second());
            };
            timePickerH->onTimeCommitted = [refreshCommitted, timePtrH]()
            {
                refreshCommitted("Time", 0, 0, 0, timePtrH->hour(), timePtrH->minute(), timePtrH->second());
            };
            dateTimePickerH->onDateTimeChanged = [refreshLive, dateTimePtrH]()
            {
                refreshLive("DateTime", dateTimePtrH->year(), dateTimePtrH->month(), dateTimePtrH->day(), dateTimePtrH->hour(), dateTimePtrH->minute(), dateTimePtrH->second());
            };
            dateTimePickerH->onDateTimeCommitted = [refreshCommitted, dateTimePtrH]()
            {
                refreshCommitted("DateTime", dateTimePtrH->year(), dateTimePtrH->month(), dateTimePtrH->day(), dateTimePtrH->hour(), dateTimePtrH->minute(), dateTimePtrH->second());
            };
            refreshLive("DateTime", dateTimePicker->year(), dateTimePicker->month(), dateTimePicker->day(), dateTimePicker->hour(), dateTimePicker->minute(), dateTimePicker->second());
            refreshCommitted("DateTime", dateTimePicker->year(), dateTimePicker->month(), dateTimePicker->day(), dateTimePicker->hour(), dateTimePicker->minute(), dateTimePicker->second());

            Vector2 numberSize = UINumberPicker::preferredSize(s);
            Vector2 numberSizeH = UINumberPicker::preferredSize(s, UIDirection::Horizontal);
            auto makeNumber = [&](UIDirection dir, const Vector2& size)
            {
                auto picker = std::make_shared<UINumberPicker>();
                paintNumber(*picker);
                picker->direction = dir;
                picker->setRange(0, 23);
                picker->wrap = true;
                picker->formatter = [](int v) { return String::format("%02d", v); };
                picker->setValue(13);
                setBox(picker, Rect(0.0f, 0.0f, size.x, size.y));
                picker->onValueChanged = [dateLivePtr](int value)
                {
                    dateLivePtr->text = String::format("Number   %02d", value);
                };
                picker->onValueCommitted = [dateCommittedPtr](int value)
                {
                    dateCommittedPtr->text = String::format("Number   %02d", value);
                };
                return picker;
            };
            auto numberPicker = makeNumber(UIDirection::Vertical, numberSize);
            auto numberPickerH = makeNumber(UIDirection::Horizontal, numberSizeH);

            auto dateSide = card(dateSideW);
            dateSide->attachChild(textOf("Preview", dateSideInner, smallPx, smallPx, mute));
            dateSide->attachChild(dateLive);
            dateSide->attachChild(dateCommitted);
            dateSide->attachChild(textOf("Wheel, click the end arrows, or drag. Arrow keys step the focused wheel.", dateSideInner, smallPx, smallPx, mute));
            dateSide->attachChild(textOf("Number vertical", dateSideInner, smallPx, smallPx, mute));
            dateSide->attachChild(numberPicker);
            dateSide->attachChild(textOf("Number horizontal", dateSideInner, smallPx, smallPx, mute));
            dateSide->attachChild(numberPickerH);
            dateSide->refit();

            float pickersH = dateSize.y;
            if (timeSize.y > pickersH) pickersH = timeSize.y;
            float pairW = dateSize.x + colGap + timeSize.x;
            float pickersH2 = dateSizeH.y;
            if (timeSizeH.y > pickersH2) pickersH2 = timeSizeH.y;
            float pairWH = dateSizeH.x + colGap + timeSizeH.x;
            float stackW = pairW;
            if (dateTimeSize.x > stackW) stackW = dateTimeSize.x;
            if (pairWH > stackW) stackW = pairWH;
            if (dateTimeSizeH.x > stackW) stackW = dateTimeSizeH.x;
            auto dateRow = hrow(pairW, pickersH, colGap);
            dateRow->attachChild(datePicker);
            dateRow->attachChild(timePicker);
            auto dateRowH = hrow(pairWH, pickersH2, colGap);
            dateRowH->attachChild(datePickerH);
            dateRowH->attachChild(timePickerH);
            auto dateColumns = hrow(stackW + colGap + dateSideW, 0.0f, colGap);
            auto dateStack = std::make_shared<UIList>();
            dateStack->direction = UIDirection::Vertical;
            dateStack->padding = 0.0f;
            dateStack->spacing = 14.0f * s;
            dateStack->color = clear;
            setBox(dateStack, Rect(0.0f, 0.0f, stackW, 0.0f));
            dateStack->attachChild(dateRow);
            dateStack->attachChild(dateTimePicker);
            dateStack->attachChild(dateRowH);
            dateStack->attachChild(dateTimePickerH);
            dateStack->refit();
            float dateBlockH = dateStack->shape.size.y;
            if (dateSide->shape.size.y > dateBlockH) dateBlockH = dateSide->shape.size.y;
            setBox(dateColumns, Rect(0.0f, 0.0f, stackW + colGap + dateSideW, dateBlockH));
            dateColumns->attachChild(dateStack);
            dateColumns->attachChild(dateSide);

            auto datePage = std::make_shared<UIList>();
            datePage->direction = UIDirection::Vertical;
            datePage->padding = pagePad;
            datePage->spacing = 14.0f * s;
            datePage->color = clear;
            setBox(datePage, Rect(0.0f, 0.0f, (float)windowWidth, 0.0f));
            datePage->attachChild(textOf("Date / Time", innerW, titlePx, titlePx, ink));
            datePage->attachChild(textOf("Vertical and horizontal wheels. Drag, scroll, click the end arrows, or use the keys.", innerW, smallPx, smallPx, mute));
            datePage->attachChild(dateColumns);
            datePage->refit();
            dateView->addChild(datePage);
            auto dateTab = makePage("Date", dateView);
            dock->dockPage(makePage("Canvas", board), UIDockMode::Fill);
            dock->dockPage(colorTab, UIDockMode::Fill);
            dock->dockPage(dateTab, UIDockMode::Fill);
            dock->activate(colorTab.get());
            mColorPicker = picker;
            root->attachChild(dock);
            mHost->prepare();
            mMenu = menu;
            mView = view;
            mDock = dock;
            mClientWidth = (float)windowWidth;
            mClientHeight = (float)windowHeight;

            auto camera = std::make_shared<Camera>();
            camera->viewport.left = 0;
            camera->viewport.top = 0;
            camera->viewport.right = (float)windowWidth;
            camera->viewport.bottom = (float)windowHeight;
            camera->viewport.front = 0.0f;
            camera->viewport.back = 1.0f;
            mSpace.clearColor = canvas;
            mSpace.add(camera);
            mSpace.activeCamera = camera;
            mSpace.add(mWindow->primitive());
            this->bindInput();
            mNativeWindow.show();
            return true;
        }

        void createFloatingWindow(UIWindow& window, const Rect& screenRect)
        {
            int w = (int)screenRect.size.x;
            int h = (int)screenRect.size.y;
            if (w < 1) w = 1;
            if (h < 1) h = 1;
            auto item = std::make_unique<FloatWindow>();
            if (!item->native.create(String(), (float)w, (float)h, NativeWindowStyle::Borderless)) {
                return;
            }
            item->native.setPosition(screenRect.origin.x, screenRect.origin.y);
            void* handle = item->native.handle();
            if (handle == nullptr) {
                return;
            }
            item->uiWindow = &window;
            item->screenRect = screenRect;
            item->surface = mRenderer.attach(handle, (uint32_t)w, (uint32_t)h);
            item->camera = std::make_shared<Camera>();
            item->camera->viewport.left = 0.0f;
            item->camera->viewport.top = 0.0f;
            item->camera->viewport.right = (float)w;
            item->camera->viewport.bottom = (float)h;
            item->camera->viewport.front = 0.0f;
            item->camera->viewport.back = 1.0f;
            item->space.clearColor = mSpace.clearColor;
            item->space.add(item->camera);
            item->space.activeCamera = item->camera;
            this->bindFloatInput(*item);
            mFloatWindows.push_back(std::move(item));
        }

        void destroyFloatingWindow(UIWindow& window)
        {
            for (auto it = mFloatWindows.begin(); it != mFloatWindows.end(); ++it)
            {
                if (it->get() == nullptr || (*it)->uiWindow != &window) continue;
                // Mouse-up on this window is still on the stack. Destroy it
                // only after that handler returns.
                (*it)->closing = true;
                (*it)->native.hide();
                mClosingFloatWindows.push_back(std::move(*it));
                mFloatWindows.erase(it);
                return;
            }
        }

        void flushClosingFloatWindows()
        {
            if (mClosingFloatWindows.empty()) return;
            auto closing = std::move(mClosingFloatWindows);
            mClosingFloatWindows.clear();
            for (auto& item : closing)
            {
                if (!item) continue;
                mRenderer.detach(item->surface);
                item->native.destroy();
            }
        }

        void placeFloatingWindows()
        {
            for (auto& item : mFloatWindows)
            {
                if (!item || item->closing || !item->native.isOpen()) continue;
                int x = (int)std::floor(item->screenRect.origin.x + 0.5f);
                int y = (int)std::floor(item->screenRect.origin.y + 0.5f);
                Vector2 placed = item->native.position();
                if ((int)placed.x == x && (int)placed.y == y) continue;
                item->native.setPosition((float)x, (float)y);
            }
        }

        void flushFloatingWindows(float delta)
        {
            if (!mHost) return;
            for (auto& item : mFloatWindows)
            {
                if (!item || item->closing || !item->native.isOpen() || !item->surface || !item->camera) continue;
                UIWindow* uiWindow = item->uiWindow;
                if (!uiWindow || !uiWindow->root()) continue;
                Vector2 client = item->native.clientSize();
                float w = client.x;
                float h = client.y;
                if (w < 1.0f) w = 1.0f;
                if (h < 1.0f) h = 1.0f;
                mHost->layout(*uiWindow, w, h);
                item->camera->viewport.right = w;
                item->camera->viewport.bottom = h;
                if (!item->shapeReady)
                {
                    item->space.add(uiWindow->primitive());
                    item->shapeReady = true;
                }
                uiWindow->tick(delta);
            }
        }

        void renderFloatingWindows()
        {
            if (!mHost) return;
            for (auto& item : mFloatWindows)
            {
                if (!item || item->closing || !item->native.isOpen() || !item->surface || !item->camera) continue;
                UIWindow* uiWindow = item->uiWindow;
                if (!uiWindow || !uiWindow->root()) continue;
                if (!item->shown)
                {
                    mRenderer.render(item->surface, item->space);
                    item->native.show(false);
                    item->shown = true;
                    continue;
                }
                mRenderer.render(item->surface, item->space);
            }
        }

        void releasePresentation()
        {
            if (mHost)
            {
                mHost->closeAll();
                this->flushClosingFloatWindows();
                mHost->flushClosing();
                if (mWindow) mHost->close(*mWindow);
                mWindow = nullptr;
                mHost.reset();
            }
            else
            {
                this->flushClosingFloatWindows();
            }
            mMenu.reset();
            mView.reset();
            mDock.reset();
            mSections.reset();
            mColorPicker.reset();
            mRenderer.detach(mSurface);
            mSurface.reset();
        }

        void quit() {
            this->releasePresentation();
            mNativeWindow.destroy();
        }

        void tick(float delta) {
            (void)delta;
            // Swapchain teardown waits for the GPU. Frame resources referenced by
            // that work must be released only after the wait.
            this->flushClosingFloatWindows();
            if (mHost) mHost->flushClosing();

            if (mSections)
            {
                mSections->refit();
            }

            if (mMenu && mDock && mMenu->shape.size.y > 0.0f)
            {
                float top = mMenu->shape.size.y;
                setBox(mDock, Rect(0.0f, top, mClientWidth, mClientHeight - top));
            }

            if (mWindow) mWindow->tick(delta);
            this->flushFloatingWindows(delta);
            if (mHost) mHost->publishAtlas();
            mRenderer.render(mSurface, mSpace);
            this->placeFloatingWindows();
            this->renderFloatingWindows();
        }

    private:
        bool splitterCursor(float x, float y, bool& vertical) const
        {
            if (!mDock) return false;
            return mDock->splitterAxis(x, y, vertical);
        }

        void hoverDock(float x, float y)
        {
            if (!mDock) return;
            mDock->setHover(x, y);
        }

        void bindFloatInput(FloatWindow& item)
        {
            FloatWindow* raw = &item;
            raw->native.onMouseMove = [raw](float x, float y)
            {
                if (raw->closing || raw->uiWindow == nullptr) return;
                raw->uiWindow->onMouseMove(x, y);
            };
            raw->native.onMouseDown = [raw](float x, float y, NativeMouseButton button)
            {
                if (raw->closing || raw->uiWindow == nullptr || button != NativeMouseButton::Left) return;
                raw->uiWindow->onMouseDown(x, y, static_cast<UIPointerCode>(static_cast<int>(button)));
            };
            raw->native.onMouseUp = [raw](float x, float y, NativeMouseButton button)
            {
                if (raw->closing || raw->uiWindow == nullptr || button != NativeMouseButton::Left) return;
                raw->uiWindow->onMouseUp(x, y, static_cast<UIPointerCode>(static_cast<int>(button)));
            };
        }

        void bindInput()
        {
            mNativeWindow.onClose = [this]()
            {
                this->releasePresentation();
            };
            mNativeWindow.onMouseMove = [this](float x, float y)
            {
                if (mColorPicker && mColorPicker->eyeDropping())
                {
                    mColorPicker->sampleEyeDropper(x, y, false);
                }
                this->hoverDock(x, y);
                if (mWindow) mWindow->onMouseMove(x, y);
            };
            mNativeWindow.onMouseDown = [this](float x, float y, NativeMouseButton button)
            {
                if (mColorPicker && mColorPicker->eyeDropping())
                {
                    if (button == NativeMouseButton::Right)
                    {
                        mColorPicker->cancelEyeDropper();
                        return;
                    }
                    if (button == NativeMouseButton::Left)
                    {
                        mColorPicker->sampleEyeDropper(x, y, true);
                        return;
                    }
                }
                if (mWindow) mWindow->onMouseDown(x, y, static_cast<UIPointerCode>(static_cast<int>(button)));
            };
            mNativeWindow.onMouseUp = [this](float x, float y, NativeMouseButton button)
            {
                if (mWindow) mWindow->onMouseUp(x, y, static_cast<UIPointerCode>(static_cast<int>(button)));
            };
            mNativeWindow.onMouseWheel = [this](float x, float y, float deltaX, float deltaY)
            {
                if (mWindow) mWindow->onMouseWheel(x, y, deltaX, deltaY);
            };
            mNativeWindow.onCursor = [this](float x, float y, NativeCursor& cursor)
            {
                bool vertical = false;
                if (!this->splitterCursor(x, y, vertical)) return false;
                cursor = vertical ? NativeCursor::SizeNorthSouth : NativeCursor::SizeWestEast;
                return true;
            };
            mNativeWindow.onKeyPress = [this](NativeKeyCode keyCode, bool, bool ctrl, bool shift, bool alt, bool repeat)
            {
                if (!mWindow) return false;
                UIKeyEvent event;
                event.key = static_cast<UIKeyCode>(static_cast<int>(keyCode));
                event.ctrl = ctrl;
                event.shift = shift;
                event.alt = alt;
                event.repeat = repeat;
                mWindow->onKeyPress(event);
                return true;
            };
            mNativeWindow.onKeyChar = [this](uint32_t character)
            {
                if (!mWindow) return false;
                mWindow->onKeyChar(character);
                return true;
            };
            mNativeWindow.onKeyRelease = [this](NativeKeyCode keyCode, bool, bool ctrl, bool shift, bool alt)
            {
                if (!mWindow) return false;
                UIKeyEvent event;
                event.key = static_cast<UIKeyCode>(static_cast<int>(keyCode));
                event.ctrl = ctrl;
                event.shift = shift;
                event.alt = alt;
                mWindow->onKeyRelease(event);
                return true;
            };
        }
    };
}
