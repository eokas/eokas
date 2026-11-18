#include "./SchemaDesigner.h"
#include "./FieldEditors.h"
#include "./FieldTypes.h"
#include "./Theme.h"
#include "./UiKit.h"

#include "UI/Main.h"
#include "UI/UIFont.h"

namespace eokas::RoseAxion {

    void RoseSchemaDesigner::bind(TableDocument& doc, const Host& host)
    {
        mDoc = &doc;
        mHost = host;
    }

    void RoseSchemaDesigner::setOverlay(UIWidget* overlay)
    {
        mOverlay = overlay;
        mHost.overlay = overlay;
        if (mEditorStruct)
        {
            mEditorStruct->setOverlay(overlay);
        }
        if (mEditorList)
        {
            mEditorList->setOverlay(overlay);
        }
        if (mEditorOptions)
        {
            mEditorOptions->setOverlay(overlay);
        }
    }

    void RoseSchemaDesigner::clearNameOverlay()
    {
        if (mOverlay != nullptr && mRenameInput != nullptr)
        {
            mOverlay->detachChild(mRenameInput);
        }
        mRenameInput = nullptr;
    }

    void RoseSchemaDesigner::reset()
    {
        mRenameSchemaName = false;
        mRenameSeed = String::empty;
        mKeepY = 0.0f;
        mEditorTop = 0.0f;
        mHeaderBar = nullptr;
        mFocus = nullptr;
        mRenameAnchor = nullptr;
        this->clearNameOverlay();
        if (mEditorStruct)
        {
            mEditorStruct->reset();
        }
        if (mEditorList)
        {
            mEditorList->reset();
        }
        if (mEditorOptions)
        {
            mEditorOptions->reset();
        }
        this->detachChildren();
    }

    const String& RoseSchemaDesigner::font() const
    {
        return mHost.font;
    }

    float RoseSchemaDesigner::scale() const
    {
        return hostScale(mHost.scale);
    }

    void RoseSchemaDesigner::notifyRebuild()
    {
        if (mHost.requestRebuild)
        {
            mHost.requestRebuild();
        }
    }

    float RoseSchemaDesigner::scrollY() const
    {
        if (mEditorStruct != nullptr && mEditorStruct->parent() == this)
        {
            return mEditorStruct->scrollY();
        }
        if (mEditorList != nullptr && mEditorList->parent() == this)
        {
            return mEditorList->scrollY();
        }
        if (mEditorOptions != nullptr && mEditorOptions->parent() == this)
        {
            return mEditorOptions->scrollY();
        }
        return mKeepY;
    }

    void RoseSchemaDesigner::setScroll(float y)
    {
        mKeepY = y;
        if (mEditorStruct != nullptr && mEditorStruct->parent() == this)
        {
            mEditorStruct->setScroll(y);
            return;
        }
        if (mEditorList != nullptr && mEditorList->parent() == this)
        {
            mEditorList->setScroll(y);
            return;
        }
        if (mEditorOptions != nullptr && mEditorOptions->parent() == this)
        {
            mEditorOptions->setScroll(y);
        }
    }

    RoseSchemaEditor_Struct::Host RoseSchemaDesigner::structHost() const
    {
        RoseSchemaEditor_Struct::Host host;
        host.requestRebuild = mHost.requestRebuild;
        host.isRebuildingUi = mHost.isRebuildingUi;
        host.writeClipboard = mHost.writeClipboard;
        host.readClipboard = mHost.readClipboard;
        host.font = mHost.font;
        host.scale = mHost.scale;
        host.overlay = mOverlay;
        return host;
    }

    RoseSchemaEditor_List::Host RoseSchemaDesigner::listHost() const
    {
        RoseSchemaEditor_List::Host host;
        host.requestRebuild = mHost.requestRebuild;
        host.isRebuildingUi = mHost.isRebuildingUi;
        host.writeClipboard = mHost.writeClipboard;
        host.readClipboard = mHost.readClipboard;
        host.font = mHost.font;
        host.scale = mHost.scale;
        host.overlay = mOverlay;
        return host;
    }

    RoseSchemaEditor_Options::Host RoseSchemaDesigner::optionsHost() const
    {
        RoseSchemaEditor_Options::Host host;
        host.requestRebuild = mHost.requestRebuild;
        host.isRebuildingUi = mHost.isRebuildingUi;
        host.writeClipboard = mHost.writeClipboard;
        host.readClipboard = mHost.readClipboard;
        host.font = mHost.font;
        host.scale = mHost.scale;
        host.overlay = mOverlay;
        return host;
    }

    void RoseSchemaDesigner::layout()
    {
        if (!visible)
        {
            return;
        }
        if (mHeaderBar != nullptr)
        {
            setBox(*mHeaderBar, Rect(0.0f, 0.0f, shape.size.x, mEditorTop));
        }
        const float editorY = mEditorTop;
        const float editorH = shape.size.y - editorY < 1.0f ? 1.0f : shape.size.y - editorY;
        if (mEditorStruct != nullptr && mEditorStruct->parent() == this)
        {
            setBox(*mEditorStruct, Rect(0.0f, editorY, shape.size.x, editorH));
            mEditorStruct->layout();
        }
        if (mEditorList != nullptr && mEditorList->parent() == this)
        {
            setBox(*mEditorList, Rect(0.0f, editorY, shape.size.x, editorH));
            mEditorList->layout();
        }
        if (mEditorOptions != nullptr && mEditorOptions->parent() == this)
        {
            setBox(*mEditorOptions, Rect(0.0f, editorY, shape.size.x, editorH));
            mEditorOptions->layout();
        }
        this->placeOverlayEditors();
        UIWidget::layout();
    }

    UIWidget* RoseSchemaDesigner::focusWidget() const
    {
        if (mEditorStruct != nullptr && mEditorStruct->parent() == this)
        {
            if (UIWidget* focus = mEditorStruct->focusWidget())
            {
                return focus;
            }
        }
        if (mEditorList != nullptr && mEditorList->parent() == this)
        {
            if (UIWidget* focus = mEditorList->focusWidget())
            {
                return focus;
            }
        }
        if (mEditorOptions != nullptr && mEditorOptions->parent() == this)
        {
            if (UIWidget* focus = mEditorOptions->focusWidget())
            {
                return focus;
            }
        }
        return mFocus;
    }

    void RoseSchemaDesigner::placeOverlayEditors()
    {
        if (mRenameInput != nullptr && mRenameAnchor != nullptr && mOverlay != nullptr)
        {
            placeOnOverlay(*mOverlay, *mRenameInput, mRenameAnchor->screenBounds());
        }
        if (mEditorStruct != nullptr && mEditorStruct->parent() == this)
        {
            mEditorStruct->placeOverlayEditors();
        }
        if (mEditorList != nullptr && mEditorList->parent() == this)
        {
            mEditorList->placeOverlayEditors();
        }
        if (mEditorOptions != nullptr && mEditorOptions->parent() == this)
        {
            mEditorOptions->placeOverlayEditors();
        }
    }

    void RoseSchemaDesigner::beginSchemaNameRename()
    {
        if (mDoc == nullptr || !mDoc->hasActiveSchema())
        {
            return;
        }
        mRenameSchemaName = true;
        mRenameSeed = mDoc->schemaName(mDoc->activeSchemaIndex());
        this->notifyRebuild();
    }

    void RoseSchemaDesigner::finishRename(const String& text)
    {
        if (mDoc == nullptr || !mRenameSchemaName)
        {
            return;
        }
        this->clearNameOverlay();
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

    void RoseSchemaDesigner::cancelRename()
    {
        if (!mRenameSchemaName)
        {
            return;
        }
        mRenameSchemaName = false;
        this->notifyRebuild();
    }

    bool RoseSchemaDesigner::handleEscape()
    {
        if (mEditorStruct != nullptr && mEditorStruct->parent() == this && mEditorStruct->handleEscape())
        {
            return true;
        }
        if (mEditorList != nullptr && mEditorList->parent() == this && mEditorList->handleEscape())
        {
            return true;
        }
        if (mEditorOptions != nullptr && mEditorOptions->parent() == this && mEditorOptions->handleEscape())
        {
            return true;
        }
        if (mRenameSchemaName)
        {
            this->cancelRename();
            return true;
        }
        return false;
    }

    void RoseSchemaDesigner::attachTypeEditor(rose::SchemaType kind, float width, float editorY, float editorH)
    {
        if (kind == rose::SchemaType::Struct)
        {
            if (!mEditorStruct)
            {
                mEditorStruct = std::make_shared<RoseSchemaEditor_Struct>();
            }
            mEditorStruct->shape.pivot = Vector2(0.0f, 0.0f);
            mEditorStruct->bind(*mDoc, this->structHost());
            mEditorStruct->setOverlay(mOverlay);
            mEditorStruct->setScroll(mKeepY);
            setBox(mEditorStruct, Rect(0.0f, editorY, width, editorH));
            mEditorStruct->rebuild();
            this->attachChild(mEditorStruct);
        }
        else if (kind == rose::SchemaType::List)
        {
            if (!mEditorList)
            {
                mEditorList = std::make_shared<RoseSchemaEditor_List>();
            }
            mEditorList->shape.pivot = Vector2(0.0f, 0.0f);
            mEditorList->bind(*mDoc, this->listHost());
            mEditorList->setOverlay(mOverlay);
            mEditorList->setScroll(mKeepY);
            setBox(mEditorList, Rect(0.0f, editorY, width, editorH));
            mEditorList->rebuild();
            this->attachChild(mEditorList);
        }
        else if (kind == rose::SchemaType::Options)
        {
            if (!mEditorOptions)
            {
                mEditorOptions = std::make_shared<RoseSchemaEditor_Options>();
            }
            mEditorOptions->shape.pivot = Vector2(0.0f, 0.0f);
            mEditorOptions->bind(*mDoc, this->optionsHost());
            mEditorOptions->setOverlay(mOverlay);
            mEditorOptions->setScroll(mKeepY);
            setBox(mEditorOptions, Rect(0.0f, editorY, width, editorH));
            mEditorOptions->rebuild();
            this->attachChild(mEditorOptions);
        }
    }

    void RoseSchemaDesigner::rebuild()
    {
        this->clearNameOverlay();
        this->detachChildren();
        mHeaderBar = nullptr;
        mEditorTop = 0.0f;
        mRenameAnchor = nullptr;
        mFocus = nullptr;
        if (mDoc == nullptr || !mDoc->isOpen() || !mDoc->hasActiveSchema())
        {
            return;
        }

        const float s = this->scale();
        const float titlePx = titleMdPx(s);
        const float inset = kRoseContentInset * s;
        const RoseTheme& t = theme();
        const Color paper = t.paper;
        const Color ink = t.ink;
        const Color accent = t.accent;
        const Color field = t.field;
        const Color clear(0.0f, 0.0f, 0.0f, 0.0f);
        const String& fontPath = this->font();
        const float width = shape.size.x;
        const float height = shape.size.y;
        fillSurface(*this, paper);
        this->shape.pivot = Vector2(0.0f, 0.0f);

        const u32_t activeIndex = mDoc->activeSchemaIndex();
        const rose::SchemaType kind = mDoc->schemaType(activeIndex);

        auto makeCard = [&]()
        {
            auto card = std::make_shared<PaintPanel>();
            card->pickable = false;
            card->fill(t.raised);
            card->hair = t.hair;
            card->framed = true;
            return card;
        };

        float hitH = 36.0f * s;
        if (hitH < titlePx + 12.0f * s)
        {
            hitH = titlePx + 12.0f * s;
        }

        auto titleCard = makeCard();
        auto chip = makeChip(schemaTypeLabel(kind), fontPath, titlePx, withAlpha(accent, t.washAlpha), accent);
        chip->pickable = false;
        float chipW = chip->shape.size.x;
        float chipH = chip->shape.size.y;
        float titleBarH = inset * 2.0f + (hitH > chipH ? hitH : chipH);
        mEditorTop = titleBarH;
        setBox(titleCard, Rect(0.0f, 0.0f, width, titleBarH));
        const float nameX = inset + chipW + 12.0f * s;
        float hitW = width - nameX - inset;
        if (hitW < 80.0f * s)
        {
            hitW = 80.0f * s;
        }
        setBox(chip, Rect(inset, (titleBarH - chipH) * 0.5f, chipW, chipH));
        titleCard->attachChild(chip);
        auto hit = std::make_shared<UIWidget>();
        hit->shape.pivot = Vector2(0.0f, 0.0f);
        hit->pickable = true;
        hit->color = clear;
        setBox(hit, Rect(nameX, (titleBarH - hitH) * 0.5f, hitW, hitH));
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
        titleCard->attachChild(hit);
        if (mRenameSchemaName)
        {
            mRenameAnchor = hit.get();
        }
        this->attachChild(titleCard);
        mHeaderBar = titleCard.get();

        float editorY = titleBarH;
        float editorH = height - titleBarH;
        if (editorH < 1.0f)
        {
            editorH = 1.0f;
        }
        this->attachTypeEditor(kind, width, editorY, editorH);

        if (mEditorStruct != nullptr && mEditorStruct->parent() == this)
        {
            if (UIWidget* focus = mEditorStruct->focusWidget())
            {
                mFocus = focus;
            }
        }
        else if (mEditorList != nullptr && mEditorList->parent() == this)
        {
            if (UIWidget* focus = mEditorList->focusWidget())
            {
                mFocus = focus;
            }
        }
        else if (mEditorOptions != nullptr && mEditorOptions->parent() == this)
        {
            if (UIWidget* focus = mEditorOptions->focusWidget())
            {
                mFocus = focus;
            }
        }

        if (mRenameSchemaName && mRenameAnchor != nullptr && mOverlay != nullptr)
        {
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
            bindCommitOnFocusLoss(*input, mHost.isRebuildingUi, [this, inputRaw]()
            {
                if (mRenameSchemaName)
                {
                    this->finishRename(String(inputRaw->text));
                }
            });
            mOverlay->attachChild(input);
            mRenameInput = input.get();
            mFocus = input.get();
        }
    }

}