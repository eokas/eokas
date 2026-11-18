#include "./SchemaEditor_Options.h"
#include "./FieldEditors.h"
#include "./Theme.h"
#include "./UiKit.h"

#include "UI/Main.h"

namespace eokas::RoseAxion {

    void RoseSchemaEditor_Options::bind(TableDocument& doc, const Host& host)
    {
        mDoc = &doc;
        mHost = host;
    }

    void RoseSchemaEditor_Options::setOverlay(UIWidget* overlay)
    {
        if (mOverlay == overlay)
        {
            return;
        }
        this->detachColorPanel();
        mOverlay = overlay;
        mHost.overlay = overlay;
    }

    void RoseSchemaEditor_Options::detachColorPanel()
    {
        if (mOverlay != nullptr && mColorPanel != nullptr)
        {
            mOverlay->detachChild(mColorPanel);
        }
        mColorPanel = nullptr;
        mColorAnchor = nullptr;
        mColorIndex = -1;
    }

    const String& RoseSchemaEditor_Options::font() const
    {
        return mHost.font;
    }

    float RoseSchemaEditor_Options::scale() const
    {
        return hostScale(mHost.scale);
    }

    void RoseSchemaEditor_Options::notifyRebuild()
    {
        if (mHost.requestRebuild)
        {
            mHost.requestRebuild();
        }
    }

    void RoseSchemaEditor_Options::reset()
    {
        mRenameIndex = -1;
        mColorIndex = -1;
        mFocusOptionIndex = -1;
        mRenameSeed = String::empty;
        mKeepY = 0.0f;
        mView = nullptr;
        mFocus = nullptr;
        this->detachColorPanel();
        this->detachChildren();
    }

    float RoseSchemaEditor_Options::scrollY() const
    {
        return mView != nullptr ? mView->scrollY() : mKeepY;
    }

    void RoseSchemaEditor_Options::setScroll(float y)
    {
        mKeepY = y;
        if (mView != nullptr)
        {
            mView->setScroll(0.0f, y);
        }
    }

    void RoseSchemaEditor_Options::layout()
    {
        if (!visible)
        {
            return;
        }
        if (mView != nullptr)
        {
            setBox(*mView, Rect(0.0f, 0.0f, shape.size.x, shape.size.y));
            mView->layout();
        }
        this->placeOverlayEditors();
        UIWidget::layout();
    }

    void RoseSchemaEditor_Options::placeOverlayEditors()
    {
        if (mOverlay != nullptr && mColorPanel != nullptr && mColorPanel->visible
            && mColorPanel->parent() == mOverlay && mColorAnchor != nullptr)
        {
            placePopup(*mOverlay, *mColorPanel, mColorAnchor->screenBounds());
        }
    }

    void RoseSchemaEditor_Options::commitOptionName(u32_t optionIndex, const String& text)
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

    bool RoseSchemaEditor_Options::handleEscape()
    {
        if (mColorIndex >= 0)
        {
            this->detachColorPanel();
            return true;
        }
        return false;
    }

    void RoseSchemaEditor_Options::rebuild()
    {
        this->detachColorPanel();
        this->detachChildren();
        mView = nullptr;
        mFocus = nullptr;
        if (mDoc == nullptr || !mDoc->isOpen() || !mDoc->hasActiveSchema())
        {
            return;
        }

        const float s = this->scale();
        const float smallPx = bodyPx(s);
        const float inset = kRoseContentInset * s;
        const float fieldH = 28.0f * s;
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

        float cardW = width;
        if (cardW < 1.0f)
        {
            cardW = 1.0f;
        }
        const float innerW = cardW - inset * 2.0f;

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
        stack->padding = 0.0f;
        stack->spacing = 0.0f;
        stack->color = clear;
        stack->shape.pivot = Vector2(0.0f, 0.0f);
        setBox(stack, Rect(0.0f, 0.0f, width, 0.0f));

        auto bodyCard = makeCard();
        auto body = std::make_shared<UIList>();
        body->direction = UIDirection::Vertical;
        body->padding = inset;
        body->spacing = 6.0f * s;
        body->color = clear;
        body->shape.pivot = Vector2(0.0f, 0.0f);
        setBox(body, Rect(0.0f, 0.0f, cardW, 0.0f));

        auto title = makeText("Options", fontPath, smallPx, mute, innerW, 18.0f * s);
        title->layout();
        setBox(title, Rect(0.0f, 0.0f, title->shape.size.x, title->shape.size.y));
        body->attachChild(title);

        const float btnW = 32.0f * s;
        const float swatchW = 28.0f * s;
        const float optGap = 6.0f * s;
        const float actionsW = swatchW + btnW * 3.0f + optGap * 3.0f;
        float nameW = innerW - actionsW - optGap;
        if (nameW < 64.0f * s)
        {
            nameW = 64.0f * s;
        }

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
            bindCommitOnFocusLoss(*name, mHost.isRebuildingUi, [this, nameRaw, i]()
            {
                if (nameRaw != nullptr)
                {
                    this->commitOptionName(static_cast<u32_t>(i), nameRaw->text);
                }
            });
            name->onFocusGain = [this, i]()
            {
                mFocusOptionIndex = i;
            };
            if (mFocusOptionIndex == i)
            {
                mFocus = name.get();
            }
            name->onSubmit = [this, i](const String& text)
            {
                this->commitOptionName(static_cast<u32_t>(i), text);
            };
            line->attachChild(name);

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

            auto up = makeButton("Up", fontPath, smallPx, btnW, fieldH, quiet, ink);
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
            auto down = makeButton("Down", fontPath, smallPx, btnW, fieldH, quiet, ink);
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
            auto remove = makeDangerButton("Del", fontPath, smallPx, btnW, fieldH);
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
        auto add = makeButton("Add Option", fontPath, smallPx, 120.0f * s, fieldH, accent, t.inkOnAccent);
        add->onClick = [this]()
        {
            if (mDoc == nullptr)
            {
                return;
            }
            auto next = mDoc->schemaOptions();
            FieldOption option;
            option.name = String::format("option_%u", static_cast<unsigned>(next.size()) + 1);
            option.label = option.name;
            option.color = formatColor(UIPalette::paletteColor(7));
            next.push_back(option);
            mDoc->setSchemaOptions(next);
            this->notifyRebuild();
        };
        body->attachChild(add);

        body->layout();
        setBox(bodyCard, Rect(0.0f, 0.0f, cardW, body->shape.size.y));
        bodyCard->attachChild(body);
        stack->attachChild(bodyCard);
        stack->layout();
        view->addChild(stack);
        view->setScroll(0.0f, mKeepY);
        this->attachChild(view);
        mView = view.get();

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
            auto optionsNow = mDoc->schemaOptions();
            if (colorIndex < static_cast<int>(optionsNow.size()))
            {
                palette->setColor(optionChipColor(optionsNow[static_cast<size_t>(colorIndex)].color));
            }
            palette->onColorChanged = [this, colorIndex](const Color& value)
            {
                if (mDoc == nullptr)
                {
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