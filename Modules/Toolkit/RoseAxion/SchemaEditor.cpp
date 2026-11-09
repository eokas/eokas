#include "./SchemaEditor.h"
#include "./Theme.h"
#include "./UiKit.h"

#include "UI/Main.h"
#include "UI/UIFont.h"

#include <cmath>
#include <utility>
#include <vector>

namespace eokas::RoseAxion {

    namespace {

        struct EscapeInput : UIInput
        {
            std::function<void()> onEscape;
            bool handleKeyPress(const UIKeyEvent& event) override
            {
                if (event.key == UIKeyCode::Escape)
                {
                    if (onEscape)
                    {
                        onEscape();
                    }
                    return true;
                }
                return UIInput::handleKeyPress(event);
            }
        };

        void placeOnOverlay(UIWidget& overlay, UIWidget& widget, const Rect& screen)
        {
            Vector2 topLeft = overlay.screenToLocal(Vector2(screen.origin.x, screen.origin.y));
            widget.shape.setBox(topLeft, screen.size);
        }

        void placePopup(UIWidget& overlay, UIWidget& menu, const Rect& anchor)
        {
            menu.layout();
            Vector2 size = menu.shape.size;
            Rect limit = overlay.screenBounds();
            float x = anchor.origin.x;
            float y = anchor.origin.y + anchor.size.y;
            float limitRight = limit.origin.x + limit.size.x;
            float limitBottom = limit.origin.y + limit.size.y;
            if (x + size.x > limitRight)
            {
                x = limitRight - size.x;
            }
            if (x < limit.origin.x)
            {
                x = limit.origin.x;
            }
            if (y + size.y > limitBottom)
            {
                y = anchor.origin.y - size.y;
            }
            if (y < limit.origin.y)
            {
                y = limit.origin.y;
            }
            Vector2 local = overlay.screenToLocal(Vector2(x, y));
            menu.shape.setBox(local, size);
        }

        const char* typeLabel(rose::SchemaType type)
        {
            if (type == rose::SchemaType::Struct)
            {
                return "Struct";
            }
            if (type == rose::SchemaType::List)
            {
                return "List";
            }
            if (type == rose::SchemaType::Options)
            {
                return "Options";
            }
            return "";
        }

    }

    void RoseSchemaEditor::bind(TableDocument& doc, const Host& host)
    {
        mDoc = &doc;
        mHost = host;
    }

    void RoseSchemaEditor::setOverlay(UIWidget* overlay)
    {
        if (mOverlay == overlay)
        {
            return;
        }
        if (mOverlay != nullptr)
        {
            if (mRenameInput != nullptr)
            {
                mOverlay->detachChild(mRenameInput);
            }
            if (mMenu != nullptr)
            {
                mOverlay->detachChild(mMenu);
            }
        }
        mRenameInput = nullptr;
        mMenu = nullptr;
        mOverlay = overlay;
    }

    const String& RoseSchemaEditor::font() const
    {
        return mHost.font;
    }

    float RoseSchemaEditor::scale() const
    {
        return mHost.scale < 0.5f ? 1.0f : mHost.scale;
    }

    void RoseSchemaEditor::notifyRebuild()
    {
        if (mHost.requestRebuild)
        {
            mHost.requestRebuild();
        }
    }

    void RoseSchemaEditor::reset()
    {
        mRenameIndex = -1;
        mTypeIndex = -2;
        mRenameSeed = String::empty;
        mKeepY = 0.0f;
        mView = nullptr;
        mOverlay = nullptr;
        mRenameAnchor = nullptr;
        mRenameInput = nullptr;
        mTypeAnchor = nullptr;
        mMenu = nullptr;
        mFocus = nullptr;
        this->detachChildren();
    }

    float RoseSchemaEditor::scrollY() const
    {
        return mView != nullptr ? mView->scrollY() : mKeepY;
    }

    void RoseSchemaEditor::setScroll(float y)
    {
        mKeepY = y;
        if (mView != nullptr)
        {
            mView->setScroll(0.0f, y);
        }
    }

    void RoseSchemaEditor::layout()
    {
        this->placeOverlayEditors();
        UIWidget::layout();
    }

    void RoseSchemaEditor::placeOverlayEditors()
    {
        if (mOverlay == nullptr)
        {
            return;
        }
        if (mRenameInput != nullptr && mRenameAnchor != nullptr)
        {
            placeOnOverlay(*mOverlay, *mRenameInput, mRenameAnchor->screenBounds());
        }
        if (mMenu != nullptr && mMenu->visible && mMenu->parent() == mOverlay)
        {
            UIMenu* menu = dynamic_cast<UIMenu*>(mMenu);
            UIWidget* host = mTypeAnchor;
            if (menu != nullptr && menu->host != nullptr)
            {
                host = menu->host;
            }
            if (host != nullptr)
            {
                placePopup(*mOverlay, *mMenu, host->screenBounds());
            }
        }
    }

    void RoseSchemaEditor::beginRename(u32_t memberIndex)
    {
        if (mDoc == nullptr || memberIndex >= mDoc->schemaMemberCount())
        {
            return;
        }
        mRenameIndex = static_cast<int>(memberIndex);
        mRenameSeed = mDoc->schemaMemberName(memberIndex);
        this->notifyRebuild();
    }

    void RoseSchemaEditor::finishRename(const String& text)
    {
        if (mDoc == nullptr || mRenameIndex < 0)
        {
            return;
        }
        int index = mRenameIndex;
        mRenameInput = nullptr;
        mRenameIndex = -1;
        if (!mDoc->renameSchemaMember(static_cast<u32_t>(index), text))
        {
            mRenameSeed = text;
            mRenameIndex = index;
            this->notifyRebuild();
            return;
        }
        this->notifyRebuild();
    }

    void RoseSchemaEditor::cancelRename()
    {
        if (mRenameIndex < 0)
        {
            return;
        }
        mRenameIndex = -1;
        this->notifyRebuild();
    }

    void RoseSchemaEditor::applyType(const String& typeName)
    {
        if (mDoc == nullptr)
        {
            return;
        }
        int index = mTypeIndex;
        mTypeIndex = -2;
        mMenu = nullptr;
        if (index == -1)
        {
            mDoc->setSchemaElementType(typeName);
        }
        else if (index >= 0)
        {
            mDoc->setSchemaMemberType(static_cast<u32_t>(index), typeName);
        }
        this->notifyRebuild();
    }

    bool RoseSchemaEditor::handleEscape()
    {
        if (mMenu != nullptr && mMenu->visible)
        {
            mMenu->visible = false;
            mTypeIndex = -2;
            return true;
        }
        if (mRenameIndex >= 0)
        {
            this->cancelRename();
            return true;
        }
        return false;
    }

    void RoseSchemaEditor::rebuild()
    {
        if (mOverlay != nullptr)
        {
            if (mRenameInput != nullptr)
            {
                mOverlay->detachChild(mRenameInput);
            }
            if (mMenu != nullptr)
            {
                mOverlay->detachChild(mMenu);
            }
        }
        mRenameInput = nullptr;
        mMenu = nullptr;
        this->detachChildren();
        mView = nullptr;
        mRenameAnchor = nullptr;
        mTypeAnchor = nullptr;
        mFocus = nullptr;
        if (mDoc == nullptr || !mDoc->isOpen() || !mDoc->hasActiveSchema())
        {
            return;
        }

        const float s = this->scale();
        const float fontPx = 15.0f * s;
        const float smallPx = 13.0f * s;
        const float headerH = 36.0f * s;
        const float rowH = 32.0f * s;
        const float pad = 12.0f * s;
        const RoseTheme& t = theme();
        const Color canvas = t.canvas;
        const Color headerBg = t.header;
        const Color ink = t.ink;
        const Color mute = t.mute;
        const Color accent = t.accent;
        const Color quiet = t.quiet;
        const Color popup = t.popup;
        const Color clear(0.0f, 0.0f, 0.0f, 0.0f);
        const Color line = t.line;
        const String& fontPath = this->font();
        const float width = shape.size.x;
        const float height = shape.size.y;
        this->color = canvas;
        this->shape.pivot = Vector2(0.0f, 0.0f);

        const rose::SchemaType kind = mDoc->schemaType(mDoc->activeSchemaIndex());
        auto header = std::make_shared<UIWidget>();
        header->shape.pivot = Vector2(0.0f, 0.0f);
        header->pickable = false;
        header->color = headerBg;
        setBox(header, Rect(0.0f, 0.0f, width, headerH));
        auto title = makeText(typeLabel(kind), fontPath, fontPx, ink, width - pad * 2.0f, 22.0f * s);
        title->layout();
        setBox(title, Rect(pad, (headerH - title->shape.size.y) * 0.5f, title->shape.size.x, title->shape.size.y));
        header->attachChild(title);
        auto headerRule = std::make_shared<UIWidget>();
        headerRule->pickable = false;
        headerRule->color = line;
        headerRule->shape.pivot = Vector2(0.0f, 0.0f);
        setBox(headerRule, Rect(0.0f, headerH - 1.0f, width, 1.0f));
        header->attachChild(headerRule);
        this->attachChild(header);

        auto showPopup = [this, s, fontPx, popup, fontPath](UIWidget& anchor, const std::vector<std::pair<String, std::function<void()>>>& entries)
        {
            if (mOverlay == nullptr)
            {
                return;
            }
            auto menu = std::make_shared<UIMenu>();
            menu->direction = UIDirection::Vertical;
            menu->padding = 4.0f * s;
            menu->spacing = 2.0f * s;
            menu->color = popup;
            menu->overlay = true;
            menu->shape.pivot = Vector2(0.0f, 0.0f);
            menu->host = &anchor;
            for (const auto& entry : entries)
            {
                auto item = std::make_shared<UIMenuItem>();
                item->paddingX = 12.0f * s;
                item->paddingY = 8.0f * s;
                item->setText(entry.first);
                styleMenuItem(*item, fontPath, fontPx);
                auto action = entry.second;
                UIMenu* menuRaw = menu.get();
                item->onClick = [this, menuRaw, action]()
                {
                    if (menuRaw != nullptr)
                    {
                        menuRaw->visible = false;
                    }
                    mMenu = nullptr;
                    action();
                };
                menu->addItem(item);
            }
            mOverlay->attachChild(menu);
            mMenu = menu.get();
            mTypeAnchor = &anchor;
            placePopup(*mOverlay, *menu, anchor.screenBounds());
        };

        float listY = headerH;
        float listH = height - headerH;
        if (listH < 1.0f)
        {
            listH = 1.0f;
        }
        auto view = std::make_shared<UIView>();
        view->shape.pivot = Vector2(0.0f, 0.0f);
        view->color = canvas;
        setBox(view, Rect(0.0f, listY, width, listH));

        auto list = std::make_shared<UIList>();
        list->direction = UIDirection::Vertical;
        list->padding = pad;
        list->spacing = 6.0f * s;
        list->color = clear;
        list->shape.pivot = Vector2(0.0f, 0.0f);

        const float innerW = width - pad * 2.0f;
        const float typeW = 110.0f * s;
        const float delW = 28.0f * s;
        const float gap = 8.0f * s;

        auto addTypeButton = [&](UIWidget& row, const String& label, int typeIndex, float x, float y)
        {
            auto button = makeButton(label, fontPath, smallPx, typeW, 24.0f * s, quiet, ink);
            setBox(button, Rect(x, y, typeW, 24.0f * s));
            UIButton* raw = button.get();
            button->onClick = [this, typeIndex, raw, showPopup]()
            {
                if (mDoc == nullptr || raw == nullptr)
                {
                    return;
                }
                mTypeIndex = typeIndex;
                std::vector<std::pair<String, std::function<void()>>> entries;
                std::vector<String> names = mDoc->schemaAssignableTypeNames();
                for (const String& name : names)
                {
                    entries.push_back({ name, [this, name]() { this->applyType(name); } });
                }
                showPopup(*raw, entries);
            };
            row.attachChild(button);
            if (mTypeIndex == typeIndex)
            {
                mTypeAnchor = button.get();
            }
            return raw;
        };

        if (kind == rose::SchemaType::Struct)
        {
            const u32_t members = mDoc->schemaMemberCount();
            for (u32_t index = 0; index < members; ++index)
            {
                const bool renaming = mRenameIndex == static_cast<int>(index);
                auto row = std::make_shared<UIWidget>();
                row->shape.pivot = Vector2(0.0f, 0.0f);
                row->color = clear;
                setBox(row, Rect(0.0f, 0.0f, innerW, rowH));
                const float nameW = innerW - typeW - delW - gap * 2.0f;
                auto nameText = makeText(mDoc->schemaMemberName(index), fontPath, smallPx, ink, nameW > 0.0f ? nameW : 1.0f, 20.0f * s);
                nameText->layout();
                float nameH = nameText->shape.size.y;
                if (nameH < 1.0f)
                {
                    nameH = 20.0f * s;
                }
                setBox(nameText, Rect(0.0f, (rowH - nameH) * 0.5f, nameW > 0.0f ? nameW : 1.0f, nameH));
                nameText->pickable = true;
                nameText->onClick = [this, index]() { this->beginRename(index); };
                row->attachChild(nameText);
                if (renaming)
                {
                    mRenameAnchor = nameText.get();
                }
                addTypeButton(*row, mDoc->schemaMemberTypeName(index), static_cast<int>(index), nameW + gap, (rowH - 24.0f * s) * 0.5f);
                auto del = makeButton("x", fontPath, smallPx, delW, 24.0f * s, quiet, ink);
                del->paddingX = 0.0f;
                del->paddingY = 0.0f;
                setBox(del, Rect(innerW - delW, (rowH - 24.0f * s) * 0.5f, delW, 24.0f * s));
                del->onClick = [this, index]()
                {
                    if (mDoc != nullptr)
                    {
                        mDoc->deleteSchemaMember(index);
                        this->notifyRebuild();
                    }
                };
                row->attachChild(del);
                list->attachChild(row);
            }
            auto add = makeButton("添加成员", fontPath, smallPx, 96.0f * s, 28.0f * s, accent, t.inkOnAccent);
            add->onClick = [this]()
            {
                if (mDoc != nullptr && mDoc->addSchemaMember())
                {
                    this->notifyRebuild();
                }
            };
            list->attachChild(add);
        }
        else if (kind == rose::SchemaType::List)
        {
            auto row = std::make_shared<UIWidget>();
            row->shape.pivot = Vector2(0.0f, 0.0f);
            row->color = clear;
            setBox(row, Rect(0.0f, 0.0f, innerW, rowH));
            auto label = makeText("元素类型", fontPath, smallPx, mute, 80.0f * s, 20.0f * s);
            label->layout();
            setBox(label, Rect(0.0f, (rowH - label->shape.size.y) * 0.5f, label->shape.size.x, label->shape.size.y));
            row->attachChild(label);
            String element = mDoc->schemaElementTypeName();
            if (element.isEmpty())
            {
                element = "String";
            }
            addTypeButton(*row, element, -1, 88.0f * s, (rowH - 24.0f * s) * 0.5f);
            list->attachChild(row);
        }
        else if (kind == rose::SchemaType::Options)
        {
            const u32_t options = mDoc->schemaOptionCount();
            for (u32_t index = 0; index < options; ++index)
            {
                auto row = std::make_shared<UIWidget>();
                row->shape.pivot = Vector2(0.0f, 0.0f);
                row->color = clear;
                setBox(row, Rect(0.0f, 0.0f, innerW, rowH));
                auto input = std::make_shared<UIInput>();
                input->shape.pivot = Vector2(0.0f, 0.0f);
                setBox(input, Rect(0.0f, (rowH - 28.0f * s) * 0.5f, innerW - delW - gap, 28.0f * s));
                input->setText(mDoc->schemaOptionLabel(index));
                UIInput* inputRaw = input.get();
                u32_t optionIndex = index;
                input->onFocusLoss = [this, inputRaw, optionIndex]()
                {
                    if (mDoc != nullptr)
                    {
                        mDoc->setSchemaOptionLabel(optionIndex, inputRaw->text);
                        this->notifyRebuild();
                    }
                };
                input->onSubmit = [this, optionIndex](const String& text)
                {
                    if (mDoc != nullptr)
                    {
                        mDoc->setSchemaOptionLabel(optionIndex, text);
                        this->notifyRebuild();
                    }
                };
                row->attachChild(input);
                auto del = makeButton("x", fontPath, smallPx, delW, 24.0f * s, quiet, ink);
                del->paddingX = 0.0f;
                del->paddingY = 0.0f;
                setBox(del, Rect(innerW - delW, (rowH - 24.0f * s) * 0.5f, delW, 24.0f * s));
                del->onClick = [this, index]()
                {
                    if (mDoc != nullptr)
                    {
                        mDoc->deleteSchemaOption(index);
                        this->notifyRebuild();
                    }
                };
                row->attachChild(del);
                list->attachChild(row);
            }
            auto add = makeButton("添加选项", fontPath, smallPx, 96.0f * s, 28.0f * s, accent, t.inkOnAccent);
            add->onClick = [this]()
            {
                if (mDoc != nullptr && mDoc->addSchemaOption())
                {
                    this->notifyRebuild();
                }
            };
            list->attachChild(add);
        }

        list->layout();
        view->addChild(list);
        view->setScroll(0.0f, mKeepY);
        this->attachChild(view);
        mView = view.get();

        if (mRenameIndex >= 0 && mRenameAnchor != nullptr && mOverlay != nullptr)
        {
            auto input = std::make_shared<EscapeInput>();
            input->shape.pivot = Vector2(0.0f, 0.0f);
            setBox(input, Rect(0.0f, 0.0f, mRenameAnchor->shape.size.x, rowH));
            input->paddingX = 8.0f * s;
            input->paddingY = 4.0f * s;
            input->background = t.paper;
            input->hoverFill = input->background;
            input->border.color = accent;
            input->border.thickness = 1.0f;
            input->caret.color = ink;
            input->selection = t.selection;
            if (UIText* label = input->label())
            {
                styleText(*label, fontPath, smallPx, ink);
            }
            input->setText(mRenameSeed);
            UIInput* inputRaw = input.get();
            input->onSubmit = [this](const String& text) { this->finishRename(text); };
            input->onEscape = [this]() { this->handleEscape(); };
            input->onFocusLoss = [this, inputRaw]()
            {
                if (mRenameIndex >= 0)
                {
                    this->finishRename(String(inputRaw->text));
                }
            };
            mOverlay->attachChild(input);
            mRenameInput = input.get();
            mFocus = input.get();
        }

        if (mTypeIndex >= 0 && mTypeAnchor != nullptr && mOverlay != nullptr && mMenu == nullptr)
        {
            std::vector<std::pair<String, std::function<void()>>> entries;
            std::vector<String> names = mDoc->schemaAssignableTypeNames();
            for (const String& name : names)
            {
                entries.push_back({ name, [this, name]() { this->applyType(name); } });
            }
            showPopup(*mTypeAnchor, entries);
        }
    }

}
