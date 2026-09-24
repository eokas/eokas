#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstring>
#include <memory>
#include <vector>

#include "ui/main.h"

namespace eokas::ui {

    class Graphics {
        Renderer mRenderer;
        Surface::Ref mSurface;
        struct FloatWindow
        {
            HWND hwnd = nullptr;
            Surface::Ref surface;
            Space space;
            Camera::Ref camera;
            UICanvas canvas;
            std::shared_ptr<UIDockPage> page;
            Rect screenRect;
        };
        std::vector<std::unique_ptr<FloatWindow>> mFloatWindows;
        std::vector<std::unique_ptr<FloatWindow>> mClosingFloatWindows;
        String mFallbackFontPath;
        std::unique_ptr<UIDockHost> mDockHost;
        Space mSpace;
        UICanvas mCanvas;
        std::shared_ptr<UIMenu> mMenu;
        std::shared_ptr<UIView> mView;
        std::shared_ptr<UIDockSpace> mDock;
        HWND mHostWindow = nullptr;
        std::shared_ptr<UILayout> mSections;
        float mClientWidth = 0.0f;
        float mClientHeight = 0.0f;

        static void fitVertical(UILayout& layout)
        {
            float height = layout.padding;
            bool any = false;
            for (auto& child : layout.children)
            {
                if (!child || !child->visible)
                {
                    continue;
                }
                if (any)
                {
                    height += layout.spacing;
                }
                any = true;
                height += child->rect.height;
            }
            height += layout.padding;
            layout.rect.height = height;
        }

    public:
        UICanvas& canvas() { return mCanvas; }

        bool splitterCursor(HWND hwnd, float x, float y, bool& vertical) const
        {
            if (hwnd != mHostWindow || !mDock) return false;
            return mDock->splitterAxis(x, y, vertical);
        }

        void hoverDock(HWND hwnd, float x, float y)
        {
            if (hwnd != mHostWindow || !mDock) return;
            mDock->setHover(x, y);
        }

        void init(HWND windowHandle, int32_t windowWidth, int32_t windowHeight) {
            mHostWindow = windowHandle;
            mRenderer.create();
            mSurface = mRenderer.attach(windowHandle, (uint32_t)windowWidth, (uint32_t)windowHeight);
            mDockHost = std::make_unique<UIDockHost>();
            mDockHost->onCreateFloatingWindow = [this](const std::shared_ptr<UIDockPage>& page, const Rect& screenRect) -> void*
            {
                return this->createFloatingWindow(page, screenRect);
            };
            mDockHost->onDestroyFloatingWindow = [this](void* handle)
            {
                this->destroyFloatingWindow(handle);
            };
            mDockHost->onPlaceFloatingWindow = [this](void* handle, const Rect& screenRect)
            {
                for (auto& item : mFloatWindows)
                {
                    if (item && item->hwnd == handle) item->screenRect = screenRect;
                }
            };
            mDockHost->onPrepareContent = [this]() { mCanvas.prepare(); };
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
                    mFallbackFontPath = path;
                    break;
                }
            }

            UINT dpi = GetDpiForWindow(windowHandle);
            if (dpi == 0)
            {
                dpi = 96;
            }
            const float uiScale = (float)dpi / 96.0f;
            const float fontPx = 15.0f * uiScale;
            const char* fontPath = "modules/test-ui/fonts/Roboto-Regular.ttf";

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
                label->rect = Rect(0.0f, 0.0f, width, height);
                label->fontPath = fontPath;
                label->fontSize = size;
                label->color = color;
                label->text = value;
                return label;
            };
            auto vCenter = [&](const std::shared_ptr<UIWidget>& child, float width, float height)
            {
                auto cell = std::make_shared<UILayout>();
                cell->direction = UILayoutDirection::Vertical;
                cell->padding = 0.0f;
                cell->spacing = 0.0f;
                cell->color = clear;
                cell->rect = Rect(0.0f, 0.0f, width, height);
                float gap = (height - child->rect.height) * 0.5f;
                if (gap < 0.0f)
                {
                    gap = 0.0f;
                }
                auto spacer = std::make_shared<UIWidget>();
                spacer->interactive = false;
                spacer->rect = Rect(0.0f, 0.0f, width, gap);
                cell->addChild(spacer);
                cell->addChild(child);
                return cell;
            };
            auto column = [&](float width)
            {
                auto layout = std::make_shared<UILayout>();
                layout->direction = UILayoutDirection::Vertical;
                layout->padding = 0.0f;
                layout->spacing = cardGap;
                layout->color = clear;
                layout->rect = Rect(0.0f, 0.0f, width, 0.0f);
                return layout;
            };
            auto card = [&](float width)
            {
                auto layout = std::make_shared<UILayout>();
                layout->direction = UILayoutDirection::Vertical;
                layout->padding = cardPad;
                layout->spacing = 12.0f * s;
                layout->color = cardColor;
                layout->rect = Rect(0.0f, 0.0f, width, 0.0f);
                return layout;
            };

            auto root = std::make_shared<UIWidget>();
            root->rect = Rect(0.0f, 0.0f, (float)windowWidth, (float)windowHeight);
            root->interactive = false;

            auto menu = std::make_shared<UIMenu>();
            menu->rect = Rect(0.0f, 0.0f, (float)windowWidth, 0.0f);
            menu->direction = UILayoutDirection::Horizontal;
            menu->padding = 4.0f * s;
            menu->spacing = 2.0f * s;
            menu->color = menuBar;

            auto status = textOf("Ready", 240.0f * s, smallPx, smallPx, mute);
            UIText* statusPtr = status.get();
            auto note = [statusPtr, ink](const char* message)
            {
                statusPtr->color = ink;
                statusPtr->text = message;
            };

            auto addItem = [&](UIMenu& target, const char* title, const Color& background) -> std::shared_ptr<UIMenuItem>
            {
                auto item = std::make_shared<UIMenuItem>();
                item->paddingX = 14.0f * s;
                item->paddingY = 8.0f * s;
                item->background = background;
                item->hoverColor = menuHover;
                item->setText(title);
                if (UIText* label = item->label())
                {
                    label->fontPath = fontPath;
                    label->fontSize = fontPx;
                    label->color = ink;
                }
                target.addItem(item);
                return item;
            };
            auto fileItem = addItem(*menu, "File", menuBar);
            auto viewItem = addItem(*menu, "View", menuBar);
            auto aboutItem = addItem(*menu, "About", menuBar);

            auto fileMenu = std::make_shared<UIMenu>();
            fileMenu->direction = UILayoutDirection::Vertical;
            fileMenu->padding = 4.0f * s;
            fileMenu->spacing = 2.0f * s;
            fileMenu->color = popup;
            fileMenu->floating = true;
            fileMenu->visible = false;
            auto newItem = addItem(*fileMenu, "New", popup);
            auto openItem = addItem(*fileMenu, "Open", popup);
            auto saveItem = addItem(*fileMenu, "Save", popup);

            fileItem->onClick = [fileItem, fileMenu]()
            {
                fileMenu->visible = !fileMenu->visible;
                fileMenu->rect.x = fileItem->rect.x;
                fileMenu->rect.y = fileItem->rect.y + fileItem->rect.height;
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

            root->children.push_back(menu);
            root->children.push_back(fileMenu);

            auto view = std::make_shared<UIView>();
            view->rect = Rect(0.0f, 0.0f, (float)windowWidth, (float)windowHeight);
            view->color = canvas;
            view->scrollbarThickness = 8.0f * s;
            view->scrollbarColor = Color(0.40f, 0.44f, 0.52f, 1.0f);
            view->scrollbarTrackColor = Color(0.078f, 0.086f, 0.110f, 1.0f);
            view->scrollbarPressedColor = accent;

            float innerW = (float)windowWidth - pagePad * 2.0f;
            float colW = (float)(int)((innerW - colGap) * 0.5f);
            float cardInner = colW - cardPad * 2.0f;
            status->rect.width = cardInner;

            auto addButton = [&](const char* title, float width, bool primary)
            {
                auto button = std::make_shared<UIButton>();
                button->rect = Rect(0.0f, 0.0f, width, buttonH);
                button->paddingX = 12.0f * s;
                button->paddingY = 8.0f * s;
                button->background = primary ? accent : neutral;
                button->hoverColor = primary ? accentHover : neutralHover;
                button->pressedColor = primary ? accentPress : neutralPress;
                button->setText(title);
                if (UIText* label = button->label())
                {
                    label->fontPath = fontPath;
                    label->fontSize = fontPx;
                    label->color = ink;
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
                auto row = std::make_shared<UILayout>();
                row->direction = UILayoutDirection::Horizontal;
                row->padding = 0.0f;
                row->spacing = spacing;
                row->color = clear;
                row->rect = Rect(0.0f, 0.0f, width, height);
                return row;
            };
            auto styleField = [&](UIInput& input)
            {
                input.paddingX = 12.0f * s;
                input.paddingY = 8.0f * s;
                input.borderThickness = s;
                input.background = field;
                input.hoverColor = fieldHover;
                input.borderColor = accent;
                input.placeholderColor = mute;
                input.caretColor = ink;
                input.selectionColor = Color(0.275f, 0.400f, 0.680f, 1.0f);
                if (UIText* label = input.label())
                {
                    label->fontPath = fontPath;
                    label->fontSize = fontPx;
                    label->color = ink;
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
                slider.trackColor = track;
                slider.fillColor = accent;
                slider.thumbColor = ink;
                slider.thumbHoverColor = accentHover;
                slider.thumbPressedColor = accentPress;
            };
            auto paintToggle = [&](UIToggle& toggle)
            {
                toggle.offColor = track;
                toggle.onColor = accent;
                toggle.offHoverColor = neutralHover;
                toggle.onHoverColor = accentHover;
                toggle.offPressedColor = neutralPress;
                toggle.onPressedColor = accentPress;
                toggle.thumbColor = ink;
            };

            auto actions = card(colW);
            actions->addChild(textOf("Actions", cardInner, smallPx, smallPx, mute));
            float buttonGap = 10.0f * s;
            float buttonW = (float)(int)((cardInner - buttonGap * 2.0f) / 3.0f);
            auto buttons = hrow(cardInner, buttonH, buttonGap);
            buttons->addChild(addButton("OK", buttonW, false));
            buttons->addChild(addButton("Cancel", buttonW, false));
            buttons->addChild(addButton("Apply", buttonW, true));
            actions->addChild(buttons);

            float toggleW = 44.0f * s;
            float toggleH = 24.0f * s;
            float toggleGap = 12.0f * s;
            auto addToggle = [&](const char* name, bool initial, const char* onText, const char* offText)
            {
                auto row = hrow(cardInner, toggleH, toggleGap);
                auto label = textOf(name, cardInner - toggleW - toggleGap, fontPx, fontPx, ink);
                auto toggle = std::make_shared<UIToggle>();
                toggle->rect = Rect(0.0f, 0.0f, toggleW, toggleH);
                toggle->value = initial;
                paintToggle(*toggle);
                toggle->onValueChanged = [note, onText, offText](bool value)
                {
                    note(value ? onText : offText);
                };
                row->addChild(vCenter(label, label->rect.width, toggleH));
                row->addChild(toggle);
                return row;
            };
            actions->addChild(addToggle("Notifications", false, "Notifications on", "Notifications off"));
            actions->addChild(addToggle("Preview", true, "Preview on", "Preview off"));
            actions->addChild(status);
            fitVertical(*actions);

            auto fields = card(colW);
            fields->spacing = 10.0f * s;
            fields->addChild(textOf("Fields", cardInner, smallPx, smallPx, mute));
            auto input = std::make_shared<UIInput>();
            input->rect = Rect(0.0f, 0.0f, cardInner, controlH);
            input->placeholder = "Title";
            input->maxLength = 64;
            styleField(*input);
            auto hint = textOf("Press Enter to commit.", cardInner, smallPx, smallPx, mute);
            UIText* hintPtr = hint.get();
            input->onSubmit = [hintPtr, ink](const String& value)
            {
                hintPtr->color = ink;
                hintPtr->text = value.isEmpty() ? String("Committed empty.") : value;
            };
            fields->addChild(input);
            fields->addChild(hint);

            auto area = std::make_shared<UIInput>();
            area->multiline = true;
            area->rect = Rect(0.0f, 0.0f, cardInner, 108.0f * s);
            area->placeholder = "\xe5\xa4\x87\xe6\xb3\xa8\xef\xbc\x8c""Ctrl+Enter \xe6\x8f\x90\xe4\xba\xa4";
            styleField(*area);
            area->setText("Keep the baseline.\n\xe7\x95\x99\xe5\x87\xba\xe4\xb8\x80\xe8\xa1\x8c\xe4\xb8\xad\xe6\x96\x87\xe3\x80\x82");
            area->onSubmit = [note](const String& value)
            {
                String line = value.replace("\n", " ");
                note(line.isEmpty() ? "Committed notes." : line.cstr());
            };
            fields->addChild(area);

            auto dropdown = std::make_shared<UIDropdown>();
            dropdown->rect = Rect(0.0f, 0.0f, cardInner, controlH);
            dropdown->paddingX = 12.0f * s;
            dropdown->paddingY = 8.0f * s;
            dropdown->borderThickness = s;
            dropdown->placeholder = "Quality";
            dropdown->background = field;
            dropdown->hoverColor = fieldHover;
            dropdown->pressedColor = accentPress;
            dropdown->borderColor = Color(0.322f, 0.361f, 0.439f, 1.0f);
            dropdown->textColor = ink;
            dropdown->placeholderColor = mute;
            dropdown->chevronColor = mute;
            dropdown->popupColor = popup;
            dropdown->itemHoverColor = menuHover;
            dropdown->itemPressedColor = accentPress;
            dropdown->itemSelectedColor = accent;
            if (UIText* caption = dropdown->caption())
            {
                caption->fontPath = fontPath;
                caption->fontSize = fontPx;
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
            fields->addChild(dropdown);
            fitVertical(*fields);

            auto sliders = card(colW);
            sliders->spacing = 16.0f * s;
            sliders->addChild(textOf("Range", cardInner, smallPx, smallPx, mute));
            float nameW = 72.0f * s;
            float valueW = 48.0f * s;
            float rowGap = 12.0f * s;
            float sliderH = 28.0f * s;
            float sliderW = cardInner - nameW - valueW - rowGap * 2.0f;
            auto levelRow = hrow(cardInner, sliderH, rowGap);
            auto levelName = textOf("Level", nameW, fontPx, fontPx, ink);
            auto level = std::make_shared<UISlider>();
            level->type = SliderType::Horizontal;
            level->rect = Rect(0.0f, 0.0f, sliderW, sliderH);
            paintSlider(*level, 4.0f * s);
            auto levelValue = textOf("0", valueW, fontPx, fontPx, ink);
            UIText* levelPtr = levelValue.get();
            level->onValueChanged = [levelPtr](float value)
            {
                levelPtr->text = String::format("%.0f", value);
            };
            level->setValue(64.0f);
            levelRow->addChild(vCenter(levelName, nameW, sliderH));
            levelRow->addChild(level);
            levelRow->addChild(vCenter(levelValue, valueW, sliderH));
            sliders->addChild(levelRow);

            float dialGap = 12.0f * s;
            float dialCol = (float)(int)((cardInner - dialGap * 2.0f) / 3.0f);
            float dialRowH = smallPx + 8.0f * s + dialCol;
            auto dialRow = hrow(cardInner, dialRowH, dialGap);
            auto makeDial = [&](SliderType type, const char* name, float initial)
            {
                auto col = std::make_shared<UILayout>();
                col->direction = UILayoutDirection::Vertical;
                col->padding = 0.0f;
                col->spacing = 8.0f * s;
                col->color = clear;
                col->rect = Rect(0.0f, 0.0f, dialCol, dialRowH);
                auto caption = textOf(name, dialCol, smallPx, smallPx, mute);
                auto slider = std::make_shared<UISlider>();
                slider->type = type;
                slider->rect = Rect(0.0f, 0.0f, dialCol, dialCol);
                bool ring = type == SliderType::Clock || type == SliderType::Angular;
                paintSlider(*slider, ring ? 6.0f * s : 4.0f * s);
                UIText* captionPtr = caption.get();
                String captionName = name;
                slider->onValueChanged = [captionPtr, captionName](float value)
                {
                    captionPtr->text = captionName + String("  ") + String::format("%.0f", value);
                };
                slider->setValue(initial);
                col->addChild(caption);
                col->addChild(slider);
                return col;
            };
            dialRow->addChild(makeDial(SliderType::Vertical, "Rise", 36.0f));
            dialRow->addChild(makeDial(SliderType::Clock, "Clock", 20.0f));
            dialRow->addChild(makeDial(SliderType::Angular, "Turn", 72.0f));
            sliders->addChild(dialRow);
            fitVertical(*sliders);

            auto sections = card(colW);
            sections->spacing = 10.0f * s;
            sections->addChild(textOf("Sections", cardInner, smallPx, smallPx, mute));
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
                region->rect = Rect(0.0f, 0.0f, cardInner, 0.0f);
                region->setSpacing(0.0f);
                region->color = clear;

                auto head = std::make_shared<UILayout>();
                head->direction = UILayoutDirection::Horizontal;
                head->rect = Rect(0.0f, 0.0f, cardInner, headH);
                head->padding = headPad;
                head->spacing = 0.0f;
                head->color = regionHead;
                head->interactive = true;

                String titleText = title;
                auto titleLabel = textOf("", cardInner - headPad * 2.0f, fontPx, fontPx, ink);
                titleLabel->text = String(open ? "- " : "+ ") + titleText;
                head->addChild(titleLabel);

                auto body = std::make_shared<UILayout>();
                body->direction = UILayoutDirection::Vertical;
                body->rect = Rect(0.0f, 0.0f, cardInner, bodyH);
                body->padding = bodyPad;
                body->spacing = bodyGap;
                body->color = regionBody;
                body->addChild(textOf("Stays in the flow while open.", bodyInner, smallPx, smallPx, mute));

                auto action = addButton("Mark", 96.0f * s, false);
                action->rect.height = bodyButtonH;
                String marked = String("Marked ") + titleText;
                action->onClick = [note, marked]()
                {
                    note(marked.cstr());
                };
                body->addChild(action);

                region->setHead(head);
                region->setBody(body);
                if (!open)
                {
                    region->setExpanded(false);
                }

                UIText* titlePtr = titleLabel.get();
                UILayout* headPtr = head.get();
                region->onExpandedChanged = [titlePtr, titleText](bool expanded)
                {
                    titlePtr->text = String(expanded ? "- " : "+ ") + titleText;
                };
                head->onPointerEnter = [headPtr, regionHeadHover]()
                {
                    headPtr->color = regionHeadHover;
                };
                head->onPointerLeave = [headPtr, regionHead]()
                {
                    headPtr->color = regionHead;
                };
                return region;
            };
            sections->addChild(addRegion("Alpha", true));
            sections->addChild(addRegion("Beta", false));
            sections->addChild(textOf("Content below follows the open section.", cardInner, smallPx, smallPx, mute));
            fitVertical(*sections);
            mSections = sections;

            auto left = column(colW);
            auto right = column(colW);
            left->addChild(actions);
            left->addChild(fields);
            right->addChild(sliders);
            right->addChild(sections);
            fitVertical(*left);
            fitVertical(*right);

            auto page = std::make_shared<UILayout>();
            page->direction = UILayoutDirection::Vertical;
            page->padding = pagePad;
            page->spacing = 14.0f * s;
            page->color = clear;
            page->rect = Rect(0.0f, 0.0f, (float)windowWidth, 0.0f);
            page->addChild(textOf("Controls", innerW, titlePx, titlePx, ink));
            page->addChild(textOf("Menus, fields, and folding sections.", innerW, smallPx, smallPx, mute));
            float columnsH = left->rect.height;
            if (right->rect.height > columnsH)
            {
                columnsH = right->rect.height;
            }
            auto columns = hrow(innerW, columnsH, colGap);
            columns->addChild(left);
            columns->addChild(right);
            page->addChild(columns);
            fitVertical(*page);
            view->addChild(page);

            mCanvas.setRoot(root);
            mCanvas.prepare();

            fileItem->syncSize();
            viewItem->syncSize();
            aboutItem->syncSize();
            float itemH = fileItem->rect.height;
            if (viewItem->rect.height > itemH)
            {
                itemH = viewItem->rect.height;
            }
            if (aboutItem->rect.height > itemH)
            {
                itemH = aboutItem->rect.height;
            }
            float menuH = menu->padding * 2.0f + itemH;
            menu->rect.height = menuH;
            float contentTop = menuH;
            float contentH = (float)windowHeight - contentTop;
            auto dock = std::make_shared<UIDockSpace>();
            dock->rect = Rect(0.0f, contentTop, (float)windowWidth, contentH);
            dock->setScreenMapper([windowHandle](const Rect& local)
            {
                POINT point;
                point.x = (LONG)local.x;
                point.y = (LONG)local.y;
                ClientToScreen(windowHandle, &point);
                return Rect((float)point.x, (float)point.y, local.width, local.height);
            });
            mDockHost->registerSpace(dock.get());
            auto headOf = [&](const char* title)
            {
                float tabH = (float)(int)(28.0f * s + 0.5f);
                float tabPx = (float)(int)(13.0f * s + 0.5f);
                float tabPad = (float)(int)(12.0f * s + 0.5f);
                if (tabH < 1.0f) tabH = 1.0f;
                if (tabPx < 1.0f) tabPx = 1.0f;
                auto head = std::make_shared<UILayout>();
                head->direction = UILayoutDirection::Horizontal;
                head->padding = tabPad;
                head->spacing = 0.0f;
                head->rect = Rect(0.0f, 0.0f, 0.0f, tabH);
                head->color = regionHead;
                head->addChild(textOf(title, tabPx, tabPx, tabPx, ink));
                return head;
            };
            auto noteBody = [&](const char* value)
            {
                auto body = std::make_shared<UILayout>();
                body->direction = UILayoutDirection::Vertical;
                body->padding = cardPad;
                body->color = regionBody;
                body->addChild(textOf(value, 280.0f * s, smallPx, smallPx, mute));
                return body;
            };
            auto makePage = [&](const char* title, const std::shared_ptr<UIWidget>& body)
            {
                auto page = std::make_shared<UIDockPage>();
                page->setHead(headOf(title));
                page->setBody(body);
                page->color = regionBody;
                page->activeColor = regionHeadHover;
                page->tabMark = accent;
                mDockHost->observe(page);
                return page;
            };
            view->rect = Rect(0.0f, 0.0f, (float)windowWidth, contentH);
            auto controls = makePage("Controls", view);
            dock->dockPage(controls, UIDockMode::Fill);
            dock->dockPage(makePage("Outline", noteBody("Drag this tab to dock or float.")), UIDockMode::Fill);
            dock->dockPage(makePage("Details", noteBody("Drop on an edge to split.")), UIDockMode::Fill);
            dock->activate(controls.get());
            root->children.push_back(dock);
            mCanvas.prepare();
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
            mSpace.add(mCanvas.shape());
        }

        void* createFloatingWindow(const std::shared_ptr<UIDockPage>& page, const Rect& screenRect)
        {
            int w = (int)screenRect.width;
            int h = (int)screenRect.height;
            if (w < 1) w = 1;
            if (h < 1) h = 1;
            HWND hwnd = CreateWindowExW(WS_EX_TOOLWINDOW, L"test-ui", L"", WS_POPUP | WS_VISIBLE,
                (int)screenRect.x, (int)screenRect.y, w, h, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)this);
            auto item = std::make_unique<FloatWindow>();
            item->hwnd = hwnd;
            item->page = page;
            item->screenRect = screenRect;
            item->surface = mRenderer.attach(hwnd, (uint32_t)w, (uint32_t)h);
            item->canvas.init((uint32_t)w, (uint32_t)h);
            if (!mFallbackFontPath.isEmpty()) item->canvas.setFallbackFontPath(mFallbackFontPath.cstr());
            if (page) page->layoutInWindow((float)w, (float)h);
            item->canvas.setRoot(page);
            item->canvas.prepare();
            item->camera = std::make_shared<Camera>();
            item->camera->viewport.left = 0.0f;
            item->camera->viewport.top = 0.0f;
            item->camera->viewport.right = (float)w;
            item->camera->viewport.bottom = (float)h;
            item->camera->viewport.front = 0.0f;
            item->camera->viewport.back = 1.0f;
            item->space.add(item->camera);
            item->space.activeCamera = item->camera;
            item->space.add(item->canvas.shape());
            mFloatWindows.push_back(std::move(item));
            return hwnd;
        }

        void destroyFloatingWindow(void* windowHandle)
        {
            HWND hwnd = (HWND)windowHandle;
            for (auto it = mFloatWindows.begin(); it != mFloatWindows.end(); ++it)
            {
                if (it->get() == nullptr || (*it)->hwnd != hwnd) continue;
                // Mouse-up on this window is still on the stack. Drop the page now,
                // and destroy the HWND only after that handler returns.
                (*it)->page.reset();
                (*it)->canvas.setRoot(nullptr);
                mClosingFloatWindows.push_back(std::move(*it));
                mFloatWindows.erase(it);
                ShowWindow(hwnd, SW_HIDE);
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
                HWND hwnd = item->hwnd;
                item->hwnd = nullptr;
                item->page.reset();
                item->canvas.setRoot(nullptr);
                item->canvas.quit();
                mRenderer.detach(item->surface);
                if (!hwnd) continue;
                SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
                DestroyWindow(hwnd);
            }
        }

        bool isClosingFloatWindow(HWND hwnd) const
        {
            for (auto& item : mClosingFloatWindows)
            {
                if (item && item->hwnd == hwnd) return true;
            }
            return false;
        }

        UICanvas* floatingCanvas(HWND hwnd)
        {
            for (auto& item : mFloatWindows)
            {
                if (item && item->hwnd == hwnd) return &item->canvas;
            }
            return nullptr;
        }

        void placeFloatingWindows()
        {
            for (auto& item : mFloatWindows)
            {
                if (!item || !item->hwnd) continue;
                SetWindowPos(item->hwnd, nullptr, (int)item->screenRect.x, (int)item->screenRect.y,
                    0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
            }
        }

        void renderFloatingWindows()
        {
            for (auto& item : mFloatWindows)
            {
                if (!item || !item->surface || !item->page || !item->camera) continue;
                RECT client = {};
                GetClientRect(item->hwnd, &client);
                float w = (float)(client.right - client.left);
                float h = (float)(client.bottom - client.top);
                if (w < 1.0f) w = 1.0f;
                if (h < 1.0f) h = 1.0f;
                item->page->layoutInWindow(w, h);
                item->camera->viewport.right = w;
                item->camera->viewport.bottom = h;
                item->canvas.flush();
                mRenderer.render(item->surface, item->space);
            }
        }

        void quit() {
            if (mDockHost)
            {
                mDockHost->closeAll();
            }
            this->flushClosingFloatWindows();
            mCanvas.quit();
            mRenderer.detach(mSurface);
        }

        void tick(float delta) {
            (void)delta;
            this->flushClosingFloatWindows();

            if (mSections)
            {
                fitVertical(*mSections);
            }

            if (mMenu && mDock && mMenu->rect.height > 0.0f)
            {
                float top = mMenu->rect.height;
                mDock->rect.x = 0.0f;
                mDock->rect.y = top;
                mDock->rect.width = mClientWidth;
                mDock->rect.height = mClientHeight - top;
            }

            mCanvas.flush();
            this->placeFloatingWindows();
            this->renderFloatingWindows();
            mRenderer.render(mSurface, mSpace);
        }
    };
}
