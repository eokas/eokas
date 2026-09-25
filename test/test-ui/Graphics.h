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
            Rect screenRect;
            bool shapeReady = false;
            bool shown = false;
        };
        std::vector<std::unique_ptr<FloatWindow>> mFloatWindows;
        std::vector<std::unique_ptr<FloatWindow>> mClosingFloatWindows;
        std::unique_ptr<UIApp> mHost;
        Space mSpace;
        UIFrame* mFrame = nullptr;
        std::shared_ptr<UIMenu> mMenu;
        std::shared_ptr<UIView> mView;
        std::shared_ptr<UIDockSpace> mDock;
        HWND mHostWindow = nullptr;
        std::shared_ptr<UIList> mSections;
        float mClientWidth = 0.0f;
        float mClientHeight = 0.0f;

    public:
        UIFrame& frame() { return *mFrame; }

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
            mHost = std::make_unique<UIApp>();
            mHost->onCreateWindow = [this](const Rect& screenRect) -> void*
            {
                return this->createFloatingWindow(screenRect);
            };
            mHost->onDestroyWindow = [this](void* handle)
            {
                this->destroyFloatingWindow(handle);
            };
            mHost->onPlaceWindow = [this](void* handle, const Rect& screenRect)
            {
                for (auto& item : mFloatWindows)
                {
                    if (item && item->hwnd == handle) item->screenRect = screenRect;
                }
            };
            mFrame = &mHost->open(windowHandle, (uint32_t)windowWidth, (uint32_t)windowHeight);
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

            UINT dpi = GetDpiForWindow(windowHandle);
            if (dpi == 0)
            {
                dpi = 96;
            }
            const float uiScale = (float)dpi / 96.0f;
            const float fontPx = 15.0f * uiScale;
            const char* fontPath = "test/test-ui/fonts/Roboto-Regular.ttf";

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
                auto cell = std::make_shared<UIList>();
                cell->direction = UIDirection::Vertical;
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
                auto layout = std::make_shared<UIList>();
                layout->direction = UIDirection::Vertical;
                layout->padding = 0.0f;
                layout->spacing = cardGap;
                layout->color = clear;
                layout->rect = Rect(0.0f, 0.0f, width, 0.0f);
                return layout;
            };
            auto card = [&](float width)
            {
                auto layout = std::make_shared<UIList>();
                layout->direction = UIDirection::Vertical;
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
            menu->direction = UIDirection::Horizontal;
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
            fileMenu->direction = UIDirection::Vertical;
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
                auto row = std::make_shared<UIList>();
                row->direction = UIDirection::Horizontal;
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
            actions->refit();

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
            fields->refit();

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
                auto col = std::make_shared<UIList>();
                col->direction = UIDirection::Vertical;
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
            sliders->refit();

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

                auto head = std::make_shared<UIList>();
                head->direction = UIDirection::Horizontal;
                head->rect = Rect(0.0f, 0.0f, cardInner, headH);
                head->padding = headPad;
                head->spacing = 0.0f;
                head->color = regionHead;
                head->interactive = true;

                String titleText = title;
                auto titleLabel = textOf("", cardInner - headPad * 2.0f, fontPx, fontPx, ink);
                titleLabel->text = String(open ? "- " : "+ ") + titleText;
                head->addChild(titleLabel);

                auto body = std::make_shared<UIList>();
                body->direction = UIDirection::Vertical;
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
                UIList* headPtr = head.get();
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
            sections->refit();
            mSections = sections;

            auto left = column(colW);
            auto right = column(colW);
            left->addChild(actions);
            left->addChild(fields);
            right->addChild(sliders);
            right->addChild(sections);
            left->refit();
            right->refit();

            auto page = std::make_shared<UIList>();
            page->direction = UIDirection::Vertical;
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
            page->refit();
            view->addChild(page);

            mFrame->setRoot(root);
            mHost->prepare();

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
            mHost->registerSpace(dock.get());
            auto headOf = [&](const char* title)
            {
                float tabH = (float)(int)(28.0f * s + 0.5f);
                float tabPx = (float)(int)(13.0f * s + 0.5f);
                float tabPad = (float)(int)(12.0f * s + 0.5f);
                if (tabH < 1.0f) tabH = 1.0f;
                if (tabPx < 1.0f) tabPx = 1.0f;
                auto head = std::make_shared<UIList>();
                head->direction = UIDirection::Horizontal;
                head->padding = tabPad;
                head->spacing = 0.0f;
                head->rect = Rect(0.0f, 0.0f, 0.0f, tabH);
                head->color = regionHead;
                head->addChild(textOf(title, tabPx, tabPx, tabPx, ink));
                return head;
            };
            auto noteBody = [&](const char* value)
            {
                auto body = std::make_shared<UIList>();
                body->direction = UIDirection::Vertical;
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
            sheetView->scrollbarColor = Color(0.40f, 0.44f, 0.52f, 1.0f);
            sheetView->scrollbarTrackColor = Color(0.078f, 0.086f, 0.110f, 1.0f);
            sheetView->scrollbarPressedColor = accent;
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
                    input->rect = Rect(0.0f, 0.0f, innerW, innerH);
                    input->paddingX = 8.0f * s;
                    input->paddingY = 8.0f * s;
                    input->borderThickness = header ? 0.0f : 1.0f;
                    input->background = header ? headerFill : cellFill;
                    input->hoverColor = header ? headerFill : fieldHover;
                    input->borderColor = accent;
                    input->placeholderColor = mute;
                    input->caretColor = ink;
                    input->selectionColor = Color(0.275f, 0.400f, 0.680f, 1.0f);
                    input->readOnly = header;
                    input->interactive = !header;
                    input->writeClipboard = writeClipboard;
                    input->readClipboard = readClipboard;
                    if (UIText* label = input->label())
                    {
                        label->fontPath = fontPath;
                        label->fontSize = smallPx;
                        label->color = header ? mute : ink;
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
                    input->hoverColor = hover;
                    cell->color = fill;
                    if (UIText* label = input->label())
                    {
                        label->color = header ? text : ink;
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
                    cell->interactive = true;
                    int index = columnHead ? column : row;
                    cell->onClick = [pick, paint, columnHead, index]()
                    {
                        pick->column = columnHead;
                        pick->index = index;
                        paint();
                    };
                    cell->onPointerEnter = [input, cell, pick, columnHead, index, headerHover]()
                    {
                        bool on = pick->index >= 0 && pick->column == columnHead && pick->index == index;
                        if (on)
                        {
                            return;
                        }
                        input->background = headerHover;
                        cell->color = headerHover;
                    };
                    cell->onPointerLeave = [paint]()
                    {
                        paint();
                    };
                }
            }
            float sheetW = headerWidth + columnWidth * (float)dataColumns;
            float sheetH = rowHeight * (float)(dataRows + 1);
            sheet->rect = Rect(0.0f, 0.0f, sheetW, sheetH);
            sheetView->addChild(sheet);
            auto detailsView = std::make_shared<UIView>();
            detailsView->color = canvas;
            detailsView->scrollbarThickness = 8.0f * s;
            detailsView->scrollbarColor = Color(0.40f, 0.44f, 0.52f, 1.0f);
            detailsView->scrollbarTrackColor = Color(0.078f, 0.086f, 0.110f, 1.0f);
            detailsView->scrollbarPressedColor = accent;
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
                input->rect = Rect(0.0f, 0.0f, innerW, innerH);
                input->paddingX = 8.0f * s;
                input->paddingY = 4.0f * s;
                input->borderThickness = 0.0f;
                input->background = cellFill;
                input->hoverColor = fieldHover;
                input->borderColor = accent;
                input->caretColor = ink;
                input->placeholderColor = mute;
                input->selectionColor = Color(0.275f, 0.400f, 0.680f, 1.0f);
                input->writeClipboard = writeClipboard;
                input->readClipboard = readClipboard;
                if (UIText* label = input->label())
                {
                    label->fontPath = fontPath;
                    label->fontSize = smallPx;
                    label->color = ink;
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
            view->rect = Rect(0.0f, 0.0f, (float)windowWidth, contentH);
            auto controls = makePage("Controls", view);
            dock->dockPage(controls, UIDockMode::Fill);
            dock->dockPage(makePage("Table", sheetView), UIDockMode::Fill);
            dock->dockPage(makePage("Outline", noteBody("Drag this tab to dock or float.")), UIDockMode::Fill);
            dock->dockPage(makePage("Details", detailsView), UIDockMode::Fill);
            dock->activate(controls.get());
            root->children.push_back(dock);
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
            mSpace.add(mFrame->shape());
        }

        void* createFloatingWindow(const Rect& screenRect)
        {
            int w = (int)screenRect.width;
            int h = (int)screenRect.height;
            if (w < 1) w = 1;
            if (h < 1) h = 1;
            HWND hwnd = CreateWindowExW(WS_EX_TOOLWINDOW, L"test-ui", L"", WS_POPUP,
                (int)screenRect.x, (int)screenRect.y, w, h, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)this);
            auto item = std::make_unique<FloatWindow>();
            item->hwnd = hwnd;
            item->screenRect = screenRect;
            item->surface = mRenderer.attach(hwnd, (uint32_t)w, (uint32_t)h);
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
            mFloatWindows.push_back(std::move(item));
            return hwnd;
        }

        void destroyFloatingWindow(void* windowHandle)
        {
            HWND hwnd = (HWND)windowHandle;
            for (auto it = mFloatWindows.begin(); it != mFloatWindows.end(); ++it)
            {
                if (it->get() == nullptr || (*it)->hwnd != hwnd) continue;
                // Mouse-up on this window is still on the stack. Destroy the HWND
                // only after that handler returns.
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

        UIFrame* floatingFrame(HWND hwnd)
        {
            if (!mHost) return nullptr;
            for (auto& item : mFloatWindows)
            {
                if (item && item->hwnd == hwnd) return mHost->find(hwnd);
            }
            return nullptr;
        }

        void placeFloatingWindows()
        {
            for (auto& item : mFloatWindows)
            {
                if (!item || !item->hwnd) continue;
                SetWindowPos(item->hwnd, nullptr, (int)item->screenRect.x, (int)item->screenRect.y,
                    0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW);
            }
        }

        void flushFloatingWindows()
        {
            if (!mHost) return;
            for (auto& item : mFloatWindows)
            {
                if (!item || !item->surface || !item->camera || !item->hwnd) continue;
                UIFrame* frame = mHost->find(item->hwnd);
                if (!frame || !frame->root()) continue;
                RECT client = {};
                GetClientRect(item->hwnd, &client);
                float w = (float)(client.right - client.left);
                float h = (float)(client.bottom - client.top);
                if (w < 1.0f) w = 1.0f;
                if (h < 1.0f) h = 1.0f;
                mHost->layout(item->hwnd, w, h);
                item->camera->viewport.right = w;
                item->camera->viewport.bottom = h;
                if (!item->shapeReady)
                {
                    item->space.add(frame->shape());
                    item->shapeReady = true;
                }
                frame->flush();
            }
        }

        void renderFloatingWindows()
        {
            if (!mHost) return;
            for (auto& item : mFloatWindows)
            {
                if (!item || !item->surface || !item->camera || !item->hwnd) continue;
                UIFrame* frame = mHost->find(item->hwnd);
                if (!frame || !frame->root()) continue;
                if (!item->shown)
                {
                    mRenderer.render(item->surface, item->space);
                    ShowWindow(item->hwnd, SW_SHOWNOACTIVATE);
                    item->shown = true;
                }
                mRenderer.render(item->surface, item->space);
            }
        }

        void quit() {
            if (mHost)
            {
                mHost->closeAll();
                this->flushClosingFloatWindows();
                mHost->flushClosing();
                mHost->close(mHostWindow);
                mFrame = nullptr;
            }
            mRenderer.detach(mSurface);
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

            if (mMenu && mDock && mMenu->rect.height > 0.0f)
            {
                float top = mMenu->rect.height;
                mDock->rect.x = 0.0f;
                mDock->rect.y = top;
                mDock->rect.width = mClientWidth;
                mDock->rect.height = mClientHeight - top;
            }

            if (mFrame) mFrame->flush();
            this->flushFloatingWindows();
            if (mHost) mHost->publishAtlas();
            mRenderer.render(mSurface, mSpace);
            this->placeFloatingWindows();
            this->renderFloatingWindows();
        }
    };
}
