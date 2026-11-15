#include "./SchemaEditor.h"
#include "./FieldEditors.h"
#include "./Theme.h"
#include "./UiKit.h"

#include "UI/Main.h"
#include "UI/UIFont.h"
#include "UI/UIStroke.h"

#include <cmath>
#include <utility>
#include <vector>

namespace eokas::RoseAxion {

    namespace {

        struct TypeDrop : UIWidget
        {
            Color field;
            Color hoverFill;
            Color pressedFill;
            UIStrokeStyle border;
            Color chevron;
            bool open = false;
            float paddingX = 8.0f;

            void layout() override
            {
                if (!visible)
                {
                    return;
                }
                if (children().empty() || !children()[0])
                {
                    return;
                }
                UIWidget& label = *children()[0];
                label.layout();
                float y = (shape.size.y - label.shape.size.y) * 0.5f;
                if (y < 0.0f)
                {
                    y = 0.0f;
                }
                this->placeChild(label, Vector2(floorf(paddingX + 0.5f), floorf(y + 0.5f)));
            }

            void render(UIPrimitive& primitive) override
            {
                if (!visible)
                {
                    return;
                }
                Matrix3 localToScreen = this->matrixLocalToScreen();
                Color bg = field;
                if (this->isPressed())
                {
                    bg = pressedFill;
                }
                else if (this->isHovered() || open)
                {
                    bg = hoverFill;
                }
                primitive.addQuad(localToScreen, Rect(0.0f, 0.0f, shape.size.x, shape.size.y), UIFont::solidUV(), bg);
                UIStroke::border(primitive, localToScreen, Rect(0.0f, 0.0f, shape.size.x, shape.size.y), border);
                float cs = shape.size.y * 0.28f;
                if (cs > 8.0f)
                {
                    cs = 8.0f;
                }
                if (cs < 4.0f)
                {
                    cs = 4.0f;
                }
                float cx = shape.size.x - paddingX - cs * 0.5f;
                float cy = shape.size.y * 0.5f;
                Rect uv = UIFont::solidUV();
                if (open)
                {
                    primitive.addQuad(
                        this->localToScreen(Vector2(cx - cs * 0.5f, cy + cs * 0.25f)),
                        this->localToScreen(Vector2(cx + cs * 0.5f, cy + cs * 0.25f)),
                        this->localToScreen(Vector2(cx, cy - cs * 0.35f)),
                        this->localToScreen(Vector2(cx, cy - cs * 0.35f)),
                        uv,
                        chevron);
                }
                else
                {
                    primitive.addQuad(
                        this->localToScreen(Vector2(cx - cs * 0.5f, cy - cs * 0.25f)),
                        this->localToScreen(Vector2(cx + cs * 0.5f, cy - cs * 0.25f)),
                        this->localToScreen(Vector2(cx, cy + cs * 0.35f)),
                        this->localToScreen(Vector2(cx, cy + cs * 0.35f)),
                        uv,
                        chevron);
                }
                UIWidget::render(primitive);
            }
        };

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
            if (mColorPanel != nullptr)
            {
                mOverlay->detachChild(mColorPanel);
            }
        }
        mRenameInput = nullptr;
        mMenu = nullptr;
        mColorPanel = nullptr;
        mOverlay = overlay;
    }

    const String& RoseSchemaEditor::font() const
    {
        return mHost.font;
    }

    float RoseSchemaEditor::scale() const
    {
        return hostScale(mHost.scale);
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
        mRenameSchemaName = false;
        mTypeIndex = -2;
        mColorIndex = -1;
        mRenameSeed = String::empty;
        mKeepY = 0.0f;
        mView = nullptr;
        mOverlay = nullptr;
        mRenameAnchor = nullptr;
        mRenameInput = nullptr;
        mTypeAnchor = nullptr;
        mMenu = nullptr;
        mColorAnchor = nullptr;
        mColorPanel = nullptr;
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
        if (mColorPanel != nullptr && mColorPanel->visible && mColorPanel->parent() == mOverlay && mColorAnchor != nullptr)
        {
            placePopup(*mOverlay, *mColorPanel, mColorAnchor->screenBounds());
        }
    }

    void RoseSchemaEditor::beginSchemaNameRename()
    {
        if (mDoc == nullptr || !mDoc->hasActiveSchema())
        {
            return;
        }
        mRenameIndex = -1;
        mRenameSchemaName = true;
        mRenameSeed = mDoc->schemaName(mDoc->activeSchemaIndex());
        this->notifyRebuild();
    }

    void RoseSchemaEditor::finishRename(const String& text)
    {
        if (mDoc == nullptr || !mRenameSchemaName)
        {
            return;
        }
        mRenameInput = nullptr;
        mRenameSchemaName = false;
        u32_t index = mDoc->activeSchemaIndex();
        if (!mDoc->renameUserSchema(index, text))
        {
            mRenameSeed = text;
            mRenameSchemaName = true;
            this->notifyRebuild();
            return;
        }
        this->notifyRebuild();
    }

    void RoseSchemaEditor::cancelRename()
    {
        if (!mRenameSchemaName)
        {
            return;
        }
        mRenameSchemaName = false;
        this->notifyRebuild();
    }

    void RoseSchemaEditor::commitMemberName(u32_t memberIndex, const String& text)
    {
        if (mDoc == nullptr)
        {
            return;
        }
        String next = trimCopy(text);
        if (mDoc->schemaMemberName(memberIndex) == next)
        {
            if (mRenameIndex == static_cast<int>(memberIndex))
            {
                mRenameIndex = -1;
            }
            return;
        }
        if (!mDoc->renameSchemaMember(memberIndex, text))
        {
            mRenameSeed = text;
            mRenameIndex = static_cast<int>(memberIndex);
            this->notifyRebuild();
            return;
        }
        if (mRenameIndex == static_cast<int>(memberIndex))
        {
            mRenameIndex = -1;
        }
        this->notifyRebuild();
    }

    void RoseSchemaEditor::commitOptionName(u32_t optionIndex, const String& text)
    {
        if (mDoc == nullptr)
        {
            return;
        }
        std::vector<FieldOption> options = mDoc->schemaOptions();
        if (optionIndex >= options.size())
        {
            return;
        }
        String next = trimCopy(text);
        if (options[optionIndex].name == next)
        {
            if (mRenameIndex == static_cast<int>(optionIndex))
            {
                mRenameIndex = -1;
            }
            return;
        }
        if (next.isEmpty())
        {
            mDoc->setStatus(DocStatus::OptionNameEmpty);
            mRenameSeed = text;
            mRenameIndex = static_cast<int>(optionIndex);
            this->notifyRebuild();
            return;
        }
        options[optionIndex].name = next;
        if (!mDoc->setSchemaOptions(options))
        {
            mRenameSeed = text;
            mRenameIndex = static_cast<int>(optionIndex);
            this->notifyRebuild();
            return;
        }
        if (mRenameIndex == static_cast<int>(optionIndex))
        {
            mRenameIndex = -1;
        }
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
        if (mColorIndex >= 0)
        {
            mColorIndex = -1;
            if (mColorPanel != nullptr)
            {
                mColorPanel->visible = false;
            }
            return true;
        }
        if (mRenameSchemaName)
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
            if (mColorPanel != nullptr)
            {
                mOverlay->detachChild(mColorPanel);
            }
        }
        mRenameInput = nullptr;
        mMenu = nullptr;
        mColorPanel = nullptr;
        this->detachChildren();
        mView = nullptr;
        mRenameAnchor = nullptr;
        mTypeAnchor = nullptr;
        mColorAnchor = nullptr;
        mFocus = nullptr;
        if (mDoc == nullptr || !mDoc->isOpen() || !mDoc->hasActiveSchema())
        {
            return;
        }

        const float s = this->scale();
        const float titlePx = 22.0f * s;
        const float fontPx = 15.0f * s;
        const float smallPx = 13.0f * s;
        const float pagePad = 16.0f * s;
        const float cardMax = 640.0f * s;
        const float inset = 14.0f * s;
        const float fieldH = 28.0f * s;
        const float rowH = 36.0f * s;
        const float typeColW = 180.0f * s;
        const RoseTheme& t = theme();
        const Color paper = t.paper;
        const Color ink = t.ink;
        const Color mute = t.mute;
        const Color accent = t.accent;
        const Color quiet = t.quiet;
        const Color popup = t.popup;
        const Color field = t.field;
        const Color clear(0.0f, 0.0f, 0.0f, 0.0f);
        const String& fontPath = this->font();
        const float width = shape.size.x;
        const float height = shape.size.y;
        fillSurface(*this, paper);
        this->shape.pivot = Vector2(0.0f, 0.0f);

        float cardW = width - pagePad * 2.0f;
        if (cardW > cardMax)
        {
            cardW = cardMax;
        }
        if (cardW < 1.0f)
        {
            cardW = 1.0f;
        }
        const float innerW = cardW - inset * 2.0f;

        const u32_t activeIndex = mDoc->activeSchemaIndex();
        const rose::SchemaType kind = mDoc->schemaType(activeIndex);

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

        auto styleField = [&](UIInput& input, float size)
        {
            input.paddingX = 8.0f * s;
            input.paddingY = 4.0f * s;
            input.background = field;
            input.hoverFill = hoverOf(field);
            input.border.color = t.ring;
            input.border.thickness = 1.0f;
            input.caret.color = ink;
            input.selection = t.selection;
            if (UIText* label = input.label())
            {
                styleText(*label, fontPath, size, ink);
            }
            input.writeClipboard = mHost.writeClipboard;
            input.readClipboard = mHost.readClipboard;
        };

        auto typeEntries = [this]()
        {
            std::vector<std::pair<String, std::function<void()>>> entries;
            if (mDoc == nullptr)
            {
                return entries;
            }
            std::vector<String> names = mDoc->schemaAssignableTypeNames();
            for (const String& name : names)
            {
                entries.push_back({ schemaLabel(name), [this, name]() { this->applyType(name); } });
            }
            return entries;
        };

        auto addTypeDrop = [&](UIWidget& row, const String& typeName, int typeIndex, float x, float y, float dropW) -> TypeDrop*
        {
            auto drop = std::make_shared<TypeDrop>();
            drop->shape.pivot = Vector2(0.0f, 0.0f);
            drop->field = field;
            drop->hoverFill = hoverOf(field);
            drop->pressedFill = pressOf(field);
            drop->border.color = t.ring;
            drop->border.thickness = 1.0f;
            drop->chevron = mute;
            drop->paddingX = 8.0f * s;
            drop->open = mTypeIndex == typeIndex;
            setBox(drop, Rect(x, y, dropW, fieldH));
            String caption = schemaLabel(typeName);
            if (caption.isEmpty())
            {
                caption = schemaLabel(String("String"));
            }
            auto label = makeText(caption, fontPath, smallPx, ink, dropW - 28.0f * s, 20.0f * s);
            label->pickable = false;
            drop->attachChild(label);
            TypeDrop* raw = drop.get();
            drop->onClick = [this, typeIndex, raw, showPopup, typeEntries]()
            {
                if (mDoc == nullptr || raw == nullptr)
                {
                    return;
                }
                mTypeIndex = typeIndex;
                raw->open = true;
                showPopup(*raw, typeEntries());
            };
            row.attachChild(drop);
            if (mTypeIndex == typeIndex)
            {
                mTypeAnchor = raw;
            }
            return raw;
        };

        auto makeCard = [&]()
        {
            auto card = std::make_shared<PaintPanel>();
            card->pickable = false;
            card->fill(t.raised);
            card->hair = t.hair;
            card->framed = true;
            return card;
        };

        auto view = std::make_shared<UIView>();
        view->shape.pivot = Vector2(0.0f, 0.0f);
        fillSurface(*view, paper);
        setBox(view, Rect(0.0f, 0.0f, width, height));

        auto stack = std::make_shared<UIList>();
        stack->direction = UIDirection::Vertical;
        stack->padding = pagePad;
        stack->spacing = 12.0f * s;
        stack->color = clear;
        stack->shape.pivot = Vector2(0.0f, 0.0f);
        setBox(stack, Rect(0.0f, 0.0f, width, 0.0f));

        auto nameCard = makeCard();
        float hitH = 36.0f * s;
        if (hitH < titlePx + 12.0f * s)
        {
            hitH = titlePx + 12.0f * s;
        }
        auto chip = makeChip(schemaTypeLabel(kind), fontPath, smallPx, withAlpha(accent, t.washAlpha), accent);
        chip->pickable = false;
        float chipW = chip->shape.size.x;
        float chipH = chip->shape.size.y;
        float nameCardH = inset * 2.0f + (hitH > chipH ? hitH : chipH);
        setBox(nameCard, Rect(0.0f, 0.0f, cardW, nameCardH));
        float hitW = cardW - inset * 2.0f - chipW - 12.0f * s;
        if (hitW < 80.0f * s)
        {
            hitW = 80.0f * s;
        }
        auto hit = std::make_shared<UIWidget>();
        hit->shape.pivot = Vector2(0.0f, 0.0f);
        hit->pickable = true;
        hit->color = clear;
        setBox(hit, Rect(inset, (nameCardH - hitH) * 0.5f, hitW, hitH));
        UIWidget* hitRaw = hit.get();
        hit->onPointerEnter = [hitRaw, hover = t.overlayHover](const UIPointerEvent&)
        {
            if (hitRaw != nullptr)
            {
                hitRaw->color = hover;
            }
        };
        hit->onPointerLeave = [hitRaw](const UIPointerEvent&)
        {
            if (hitRaw != nullptr)
            {
                hitRaw->color = Color(0.0f, 0.0f, 0.0f, 0.0f);
            }
        };
        hit->onClick = [this]() { this->beginSchemaNameRename(); };
        auto nameText = makeText(mDoc->schemaName(activeIndex), fontPath, titlePx, ink, hitW - 8.0f * s, hitH);
        nameText->pickable = false;
        nameText->layout();
        float nameH = nameText->shape.size.y;
        if (nameH < 1.0f)
        {
            nameH = titlePx;
        }
        setBox(nameText, Rect(4.0f * s, (hitH - nameH) * 0.5f, hitW - 8.0f * s, nameH));
        hit->attachChild(nameText);
        auto nameRule = std::make_shared<UIWidget>();
        nameRule->pickable = false;
        nameRule->color = withAlpha(ink, 0.18f);
        nameRule->shape.pivot = Vector2(0.0f, 0.0f);
        setBox(nameRule, Rect(4.0f * s, hitH - 2.0f * s, hitW - 8.0f * s, 1.0f));
        hit->attachChild(nameRule);
        nameCard->attachChild(hit);
        if (mRenameSchemaName)
        {
            mRenameAnchor = hit.get();
        }
        setBox(chip, Rect(cardW - inset - chipW, (nameCardH - chipH) * 0.5f, chipW, chipH));
        nameCard->attachChild(chip);
        stack->attachChild(nameCard);

        auto bodyCard = makeCard();
        auto body = std::make_shared<UIList>();
        body->direction = UIDirection::Vertical;
        body->padding = inset;
        body->spacing = 6.0f * s;
        body->color = clear;
        body->shape.pivot = Vector2(0.0f, 0.0f);
        setBox(body, Rect(0.0f, 0.0f, cardW, 0.0f));

        if (kind == rose::SchemaType::Struct)
        {
            const float btnW = 32.0f * s;
            const float swatchW = 28.0f * s;
            const float optGap = 6.0f * s;
            const float memberTypeW = 140.0f * s;
            const float actionsW = memberTypeW + swatchW + btnW * 3.0f + optGap * 4.0f;
            float nameW = (innerW - actionsW - optGap * 2.0f) * 0.5f;
            if (nameW < 64.0f * s)
            {
                nameW = 64.0f * s;
            }
            const float labelW = nameW;

            auto cols = std::make_shared<UIWidget>();
            cols->shape.pivot = Vector2(0.0f, 0.0f);
            cols->pickable = false;
            cols->color = clear;
            setBox(cols, Rect(0.0f, 0.0f, innerW, 18.0f * s));
            auto hName = makeText("名称", fontPath, smallPx, mute, nameW, 16.0f * s);
            hName->layout();
            setBox(hName, Rect(0.0f, 0.0f, hName->shape.size.x, hName->shape.size.y));
            auto hLabel = makeText("文本", fontPath, smallPx, mute, labelW, 16.0f * s);
            hLabel->layout();
            setBox(hLabel, Rect(nameW + optGap, 0.0f, hLabel->shape.size.x, hLabel->shape.size.y));
            auto hType = makeText("类型", fontPath, smallPx, mute, memberTypeW, 16.0f * s);
            hType->layout();
            setBox(hType, Rect(nameW + optGap + labelW + optGap, 0.0f, hType->shape.size.x, hType->shape.size.y));
            cols->attachChild(hName);
            cols->attachChild(hLabel);
            cols->attachChild(hType);
            body->attachChild(cols);

            const u32_t members = mDoc->schemaMemberCount();
            if (mRenameIndex >= 0 && static_cast<u32_t>(mRenameIndex) >= members)
            {
                mRenameIndex = -1;
                mRenameSeed = String::empty;
            }
            if (mColorIndex >= static_cast<int>(members))
            {
                mColorIndex = -1;
            }
            for (u32_t index = 0; index < members; ++index)
            {
                const int i = static_cast<int>(index);
                auto line = std::make_shared<UIList>();
                line->direction = UIDirection::Horizontal;
                line->padding = 0.0f;
                line->spacing = optGap;
                line->color = clear;
                line->shape.pivot = Vector2(0.0f, 0.0f);

                auto name = std::make_shared<UIInput>();
                name->shape.pivot = Vector2(0.0f, 0.0f);
                setBox(name, Rect(0.0f, 0.0f, nameW, fieldH));
                styleField(*name, smallPx);
                if (mRenameIndex == i)
                {
                    name->setText(mRenameSeed);
                }
                else
                {
                    name->setText(mDoc->schemaMemberName(index));
                }
                UIInput* nameRaw = name.get();
                name->onFocusLoss = [this, nameRaw, index]()
                {
                    if (mDoc != nullptr && nameRaw != nullptr)
                    {
                        this->commitMemberName(index, nameRaw->text);
                    }
                };
                name->onSubmit = [this, index](const String& text)
                {
                    this->commitMemberName(index, text);
                };
                line->attachChild(name);

                auto label = std::make_shared<UIInput>();
                label->shape.pivot = Vector2(0.0f, 0.0f);
                setBox(label, Rect(0.0f, 0.0f, labelW, fieldH));
                styleField(*label, smallPx);
                label->setText(mDoc->schemaMemberLabel(index));
                UIInput* labelRaw = label.get();
                label->onFocusLoss = [this, labelRaw, index]()
                {
                    if (mDoc == nullptr || labelRaw == nullptr)
                    {
                        return;
                    }
                    mDoc->setSchemaMemberLabel(index, labelRaw->text);
                    this->notifyRebuild();
                };
                label->onSubmit = [this, index](const String& text)
                {
                    if (mDoc == nullptr)
                    {
                        return;
                    }
                    mDoc->setSchemaMemberLabel(index, text);
                    this->notifyRebuild();
                };
                line->attachChild(label);

                addTypeDrop(*line, mDoc->schemaMemberTypeName(index), i, 0.0f, 0.0f, memberTypeW);

                Color chip = optionChipColor(mDoc->schemaMemberColor(index));
                auto swatch = makeButton("", fontPath, smallPx, swatchW, swatchW, chip, ink);
                swatch->circle = true;
                swatch->paddingX = 0.0f;
                swatch->paddingY = 0.0f;
                swatch->onClick = [this, i]()
                {
                    mColorIndex = mColorIndex == i ? -1 : i;
                    this->notifyRebuild();
                };
                if (mColorIndex == i)
                {
                    mColorAnchor = swatch.get();
                }
                line->attachChild(swatch);

                auto up = makeButton("上", fontPath, smallPx, btnW, fieldH, quiet, ink);
                up->onClick = [this, i]()
                {
                    if (mDoc == nullptr || i <= 0)
                    {
                        return;
                    }
                    mDoc->moveSchemaMember(static_cast<u32_t>(i), -1);
                    mColorIndex = -1;
                    this->notifyRebuild();
                };
                auto down = makeButton("下", fontPath, smallPx, btnW, fieldH, quiet, ink);
                down->onClick = [this, i]()
                {
                    if (mDoc == nullptr || i + 1 >= static_cast<int>(mDoc->schemaMemberCount()))
                    {
                        return;
                    }
                    mDoc->moveSchemaMember(static_cast<u32_t>(i), 1);
                    mColorIndex = -1;
                    this->notifyRebuild();
                };
                auto remove = makeDangerButton("删", fontPath, smallPx, btnW, fieldH);
                remove->onClick = [this, i]()
                {
                    if (mDoc == nullptr)
                    {
                        return;
                    }
                    mDoc->deleteSchemaMember(static_cast<u32_t>(i));
                    mColorIndex = -1;
                    this->notifyRebuild();
                };
                line->attachChild(up);
                line->attachChild(down);
                line->attachChild(remove);
                body->attachChild(line);
            }
            auto add = makeButton("添加成员", fontPath, smallPx, 96.0f * s, fieldH, t.ghost, ink);
            add->onClick = [this]()
            {
                if (mDoc == nullptr || !mDoc->addSchemaMember())
                {
                    return;
                }
                u32_t index = mDoc->schemaMemberCount() - 1;
                mDoc->setSchemaMemberLabel(index, String::format("成员%d", static_cast<int>(index) + 1));
                mDoc->setSchemaMemberColor(index, formatColor(UIPalette::paletteColor(7)));
                this->notifyRebuild();
            };
            body->attachChild(add);
        }
        else if (kind == rose::SchemaType::List)
        {
            auto title = makeText("元素类型", fontPath, smallPx, mute, innerW, 18.0f * s);
            title->layout();
            setBox(title, Rect(0.0f, 0.0f, title->shape.size.x, title->shape.size.y));
            body->attachChild(title);
            auto row = std::make_shared<UIWidget>();
            row->shape.pivot = Vector2(0.0f, 0.0f);
            row->color = clear;
            setBox(row, Rect(0.0f, 0.0f, innerW, rowH));
            String element = mDoc->schemaElementTypeName();
            if (element.isEmpty())
            {
                element = "String";
            }
            addTypeDrop(*row, element, -1, 0.0f, (rowH - fieldH) * 0.5f, typeColW);
            body->attachChild(row);
        }
        else if (kind == rose::SchemaType::Options)
        {
            auto title = makeText("选项", fontPath, smallPx, mute, innerW, 18.0f * s);
            title->layout();
            setBox(title, Rect(0.0f, 0.0f, title->shape.size.x, title->shape.size.y));
            body->attachChild(title);

            const float btnW = 32.0f * s;
            const float swatchW = 28.0f * s;
            const float optGap = 6.0f * s;
            const float actionsW = swatchW + btnW * 3.0f + optGap * 3.0f;
            float nameW = (innerW - actionsW - optGap * 2.0f) * 0.5f;
            if (nameW < 64.0f * s)
            {
                nameW = 64.0f * s;
            }
            const float labelW = nameW;

            auto cols = std::make_shared<UIWidget>();
            cols->shape.pivot = Vector2(0.0f, 0.0f);
            cols->pickable = false;
            cols->color = clear;
            setBox(cols, Rect(0.0f, 0.0f, innerW, 18.0f * s));
            auto hName = makeText("名称", fontPath, smallPx, mute, nameW, 16.0f * s);
            hName->layout();
            setBox(hName, Rect(0.0f, 0.0f, hName->shape.size.x, hName->shape.size.y));
            auto hLabel = makeText("文本", fontPath, smallPx, mute, labelW, 16.0f * s);
            hLabel->layout();
            setBox(hLabel, Rect(nameW + optGap, 0.0f, hLabel->shape.size.x, hLabel->shape.size.y));
            cols->attachChild(hName);
            cols->attachChild(hLabel);
            body->attachChild(cols);

            std::vector<FieldOption> options = mDoc->schemaOptions();
            if (mRenameIndex >= 0 && static_cast<u32_t>(mRenameIndex) >= options.size())
            {
                mRenameIndex = -1;
                mRenameSeed = String::empty;
            }
            if (mColorIndex >= static_cast<int>(options.size()))
            {
                mColorIndex = -1;
            }
            for (int i = 0; i < static_cast<int>(options.size()); ++i)
            {
                auto line = std::make_shared<UIList>();
                line->direction = UIDirection::Horizontal;
                line->padding = 0.0f;
                line->spacing = optGap;
                line->color = clear;
                line->shape.pivot = Vector2(0.0f, 0.0f);

                auto name = std::make_shared<UIInput>();
                name->shape.pivot = Vector2(0.0f, 0.0f);
                setBox(name, Rect(0.0f, 0.0f, nameW, fieldH));
                styleField(*name, smallPx);
                if (mRenameIndex == i)
                {
                    name->setText(mRenameSeed);
                }
                else
                {
                    name->setText(options[static_cast<size_t>(i)].name);
                }
                UIInput* nameRaw = name.get();
                name->onFocusLoss = [this, nameRaw, i]()
                {
                    if (nameRaw != nullptr)
                    {
                        this->commitOptionName(static_cast<u32_t>(i), nameRaw->text);
                    }
                };
                name->onSubmit = [this, i](const String& text)
                {
                    this->commitOptionName(static_cast<u32_t>(i), text);
                };
                line->attachChild(name);

                auto label = std::make_shared<UIInput>();
                label->shape.pivot = Vector2(0.0f, 0.0f);
                setBox(label, Rect(0.0f, 0.0f, labelW, fieldH));
                styleField(*label, smallPx);
                label->setText(options[static_cast<size_t>(i)].label);
                UIInput* labelRaw = label.get();
                label->onFocusLoss = [this, labelRaw, i]()
                {
                    if (mDoc == nullptr || labelRaw == nullptr)
                    {
                        return;
                    }
                    auto next = mDoc->schemaOptions();
                    if (i < 0 || i >= static_cast<int>(next.size()))
                    {
                        return;
                    }
                    next[static_cast<size_t>(i)].label = labelRaw->text;
                    mDoc->setSchemaOptions(next);
                    this->notifyRebuild();
                };
                label->onSubmit = [this, i](const String& text)
                {
                    if (mDoc == nullptr)
                    {
                        return;
                    }
                    auto next = mDoc->schemaOptions();
                    if (i < 0 || i >= static_cast<int>(next.size()))
                    {
                        return;
                    }
                    next[static_cast<size_t>(i)].label = text;
                    mDoc->setSchemaOptions(next);
                    this->notifyRebuild();
                };
                line->attachChild(label);

                Color chip = optionChipColor(options[static_cast<size_t>(i)].color);
                auto swatch = makeButton("", fontPath, smallPx, swatchW, swatchW, chip, ink);
                swatch->circle = true;
                swatch->paddingX = 0.0f;
                swatch->paddingY = 0.0f;
                swatch->onClick = [this, i]()
                {
                    mColorIndex = mColorIndex == i ? -1 : i;
                    this->notifyRebuild();
                };
                if (mColorIndex == i)
                {
                    mColorAnchor = swatch.get();
                }
                line->attachChild(swatch);

                auto up = makeButton("上", fontPath, smallPx, btnW, fieldH, quiet, ink);
                up->onClick = [this, i]()
                {
                    if (mDoc == nullptr || i <= 0)
                    {
                        return;
                    }
                    auto next = mDoc->schemaOptions();
                    if (i >= static_cast<int>(next.size()))
                    {
                        return;
                    }
                    std::swap(next[static_cast<size_t>(i)], next[static_cast<size_t>(i - 1)]);
                    mDoc->setSchemaOptions(next);
                    mColorIndex = -1;
                    this->notifyRebuild();
                };
                auto down = makeButton("下", fontPath, smallPx, btnW, fieldH, quiet, ink);
                down->onClick = [this, i]()
                {
                    if (mDoc == nullptr)
                    {
                        return;
                    }
                    auto next = mDoc->schemaOptions();
                    if (i + 1 >= static_cast<int>(next.size()))
                    {
                        return;
                    }
                    std::swap(next[static_cast<size_t>(i)], next[static_cast<size_t>(i + 1)]);
                    mDoc->setSchemaOptions(next);
                    mColorIndex = -1;
                    this->notifyRebuild();
                };
                auto remove = makeDangerButton("删", fontPath, smallPx, btnW, fieldH);
                remove->onClick = [this, i]()
                {
                    if (mDoc == nullptr)
                    {
                        return;
                    }
                    auto next = mDoc->schemaOptions();
                    if (i < 0 || i >= static_cast<int>(next.size()))
                    {
                        return;
                    }
                    next.erase(next.begin() + i);
                    mDoc->setSchemaOptions(next);
                    mColorIndex = -1;
                    this->notifyRebuild();
                };
                line->attachChild(up);
                line->attachChild(down);
                line->attachChild(remove);
                body->attachChild(line);
            }
            auto add = makeButton("添加选项", fontPath, smallPx, 120.0f * s, fieldH, accent, t.inkOnAccent);
            add->onClick = [this]()
            {
                if (mDoc == nullptr)
                {
                    return;
                }
                auto next = mDoc->schemaOptions();
                FieldOption option;
                option.label = String::format("选项%d", static_cast<int>(next.size()) + 1);
                option.color = formatColor(UIPalette::paletteColor(7));
                next.push_back(option);
                mDoc->setSchemaOptions(next);
                this->notifyRebuild();
            };
            body->attachChild(add);
        }

        body->layout();
        setBox(bodyCard, Rect(0.0f, 0.0f, cardW, body->shape.size.y));
        bodyCard->attachChild(body);
        stack->attachChild(bodyCard);

        stack->layout();
        view->addChild(stack);
        view->setScroll(0.0f, mKeepY);
        this->attachChild(view);
        mView = view.get();

        if (mRenameSchemaName && mRenameAnchor != nullptr && mOverlay != nullptr)
        {
            auto input = std::make_shared<EscapeInput>();
            input->shape.pivot = Vector2(0.0f, 0.0f);
            setBox(input, Rect(0.0f, 0.0f, mRenameAnchor->shape.size.x, mRenameAnchor->shape.size.y));
            styleField(*input, titlePx);
            input->background = t.paper;
            input->hoverFill = input->background;
            input->border.color = accent;
            input->setText(mRenameSeed);
            UIInput* inputRaw = input.get();
            input->onSubmit = [this](const String& text) { this->finishRename(text); };
            input->onEscape = [this]() { this->handleEscape(); };
            input->onFocusLoss = [this, inputRaw]()
            {
                if (mRenameSchemaName)
                {
                    this->finishRename(String(inputRaw->text));
                }
            };
            mOverlay->attachChild(input);
            mRenameInput = input.get();
            mFocus = input.get();
        }

        if (mTypeIndex >= -1 && mTypeAnchor != nullptr && mOverlay != nullptr && mMenu == nullptr)
        {
            showPopup(*mTypeAnchor, typeEntries());
        }

        if (mColorIndex >= 0 && mColorAnchor != nullptr && mOverlay != nullptr)
        {
            const float palettePad = 4.0f;
            const float paletteGap = 2.0f;
            const float cellSize = 20.0f;
            Vector2 paletteSize = UIPalette::preferredSize(s, palettePad, paletteGap, cellSize);
            auto palette = std::make_shared<UIPalette>();
            palette->overlay = true;
            palette->uiScale = s;
            palette->padding = palettePad;
            palette->spacing = paletteGap;
            palette->cellSize = cellSize;
            palette->background = popup;
            palette->accent = accent;
            palette->line = t.line;
            setBox(palette, Rect(0.0f, 0.0f, paletteSize.x, paletteSize.y));
            const int colorIndex = mColorIndex;
            const bool memberColor = kind == rose::SchemaType::Struct;
            if (memberColor)
            {
                if (colorIndex >= 0 && static_cast<u32_t>(colorIndex) < mDoc->schemaMemberCount())
                {
                    palette->setColor(optionChipColor(mDoc->schemaMemberColor(static_cast<u32_t>(colorIndex))));
                }
            }
            else
            {
                auto optionsNow = mDoc->schemaOptions();
                if (colorIndex < static_cast<int>(optionsNow.size()))
                {
                    palette->setColor(optionChipColor(optionsNow[static_cast<size_t>(colorIndex)].color));
                }
            }
            palette->onColorChanged = [this, colorIndex, memberColor](const Color& value)
            {
                if (mDoc == nullptr)
                {
                    return;
                }
                if (memberColor)
                {
                    if (colorIndex < 0 || static_cast<u32_t>(colorIndex) >= mDoc->schemaMemberCount())
                    {
                        return;
                    }
                    mDoc->setSchemaMemberColor(static_cast<u32_t>(colorIndex), formatColor(value));
                    this->notifyRebuild();
                    return;
                }
                auto next = mDoc->schemaOptions();
                if (colorIndex < 0 || colorIndex >= static_cast<int>(next.size()))
                {
                    return;
                }
                next[static_cast<size_t>(colorIndex)].color = formatColor(value);
                mDoc->setSchemaOptions(next);
                this->notifyRebuild();
            };
            UIPalette* paletteRaw = palette.get();
            palette->onPointerPressAnywhere = [this, paletteRaw](const UIPointerEvent& event)
            {
                if (mColorIndex < 0 || paletteRaw == nullptr)
                {
                    return;
                }
                bool inside = paletteRaw->pick(paletteRaw->screenToParentPivot(event.position)) != nullptr;
                if (!inside && mColorAnchor != nullptr)
                {
                    inside = mColorAnchor->pick(mColorAnchor->screenToParentPivot(event.position)) != nullptr;
                }
                if (!inside)
                {
                    mColorIndex = -1;
                    paletteRaw->visible = false;
                }
            };
            mOverlay->attachChild(palette);
            mColorPanel = palette.get();
            placePopup(*mOverlay, *palette, mColorAnchor->screenBounds());
        }
    }

}
