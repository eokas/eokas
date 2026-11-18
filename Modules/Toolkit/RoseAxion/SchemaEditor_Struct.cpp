#include "./SchemaEditor_Struct.h"
#include "./Theme.h"
#include "./UiKit.h"

#include "UI/Main.h"
#include "UI/UIFont.h"

namespace eokas::RoseAxion {

    void RoseSchemaEditor_Struct::bind(TableDocument& doc, const Host& host)
    {
        mDoc = &doc;
        mHost = host;
    }

    void RoseSchemaEditor_Struct::setOverlay(UIWidget* overlay)
    {
        if (mOverlay == overlay)
        {
            return;
        }
        this->detachTypeMenu();
        mOverlay = overlay;
        mHost.overlay = overlay;
    }

    void RoseSchemaEditor_Struct::detachTypeMenu()
    {
        mTypeMenu.close(mOverlay);
    }

    const String& RoseSchemaEditor_Struct::font() const
    {
        return mHost.font;
    }

    float RoseSchemaEditor_Struct::scale() const
    {
        return hostScale(mHost.scale);
    }

    void RoseSchemaEditor_Struct::notifyRebuild()
    {
        if (mHost.requestRebuild)
        {
            mHost.requestRebuild();
        }
    }

    void RoseSchemaEditor_Struct::reset()
    {
        mRenameIndex = -1;
        mTypeIndex = -2;
        mFocusMemberIndex = -1;
        mRenameSeed = String::empty;
        mKeepY = 0.0f;
        mView = nullptr;
        mFocus = nullptr;
        this->detachTypeMenu();
        this->detachChildren();
    }

    float RoseSchemaEditor_Struct::scrollY() const
    {
        return mView != nullptr ? mView->scrollY() : mKeepY;
    }

    void RoseSchemaEditor_Struct::setScroll(float y)
    {
        mKeepY = y;
        if (mView != nullptr)
        {
            mView->setScroll(0.0f, y);
        }
    }

    void RoseSchemaEditor_Struct::layout()
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

    void RoseSchemaEditor_Struct::placeOverlayEditors()
    {
        mTypeMenu.place(mOverlay);
    }

    void RoseSchemaEditor_Struct::commitMemberName(u32_t memberIndex, const String& text)
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

    void RoseSchemaEditor_Struct::applyMemberType(u32_t memberIndex, const String& typeName)
    {
        if (mDoc == nullptr)
        {
            return;
        }
        mTypeIndex = -2;
        this->detachTypeMenu();
        mDoc->setSchemaMemberType(memberIndex, typeName);
        this->notifyRebuild();
    }

    bool RoseSchemaEditor_Struct::handleEscape()
    {
        if (mTypeMenu.visible() || mTypeIndex >= 0)
        {
            mTypeIndex = -2;
            this->detachTypeMenu();
            this->notifyRebuild();
            return true;
        }
        return false;
    }

    void RoseSchemaEditor_Struct::rebuild()
    {
        this->detachTypeMenu();
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
        const Color quiet = t.quiet;
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

        const float btnW = 32.0f * s;
        const float optGap = 6.0f * s;
        const float memberTypeW = 140.0f * s;
        const float actionsW = memberTypeW + btnW * 3.0f + optGap * 3.0f;
        float nameW = innerW - actionsW - optGap;
        if (nameW < 64.0f * s)
        {
            nameW = 64.0f * s;
        }

        auto cols = std::make_shared<UIWidget>();
        cols->shape.pivot = Vector2(0.0f, 0.0f);
        cols->pickable = false;
        cols->color = clear;
        setBox(cols, Rect(0.0f, 0.0f, innerW, 18.0f * s));
        auto hName = makeText("Name", fontPath, smallPx, mute, nameW, 16.0f * s);
        hName->layout();
        setBox(hName, Rect(0.0f, 0.0f, hName->shape.size.x, hName->shape.size.y));
        auto hType = makeText("Type", fontPath, smallPx, mute, memberTypeW, 16.0f * s);
        hType->layout();
        setBox(hType, Rect(nameW + optGap, 0.0f, hType->shape.size.x, hType->shape.size.y));
        cols->attachChild(hName);
        cols->attachChild(hType);
        body->attachChild(cols);

        const u32_t members = mDoc->schemaMemberCount();
        if (mRenameIndex >= 0 && static_cast<u32_t>(mRenameIndex) >= members)
        {
            mRenameIndex = -1;
            mRenameSeed = String::empty;
        }

        auto openTypeMenu = [this](int slot)
        {
            mTypeIndex = slot;
        };

        auto onPickType = [this](const String& name)
        {
            const int slot = mTypeIndex;
            if (slot >= 0)
            {
                this->applyMemberType(static_cast<u32_t>(slot), name);
            }
        };

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
            bindCommitOnFocusLoss(*name, mHost.isRebuildingUi, [this, nameRaw, index]()
            {
                if (mDoc != nullptr && nameRaw != nullptr)
                {
                    this->commitMemberName(index, nameRaw->text);
                }
            });
            name->onFocusGain = [this, i]()
            {
                mFocusMemberIndex = i;
            };
            if (mFocusMemberIndex == i)
            {
                mFocus = name.get();
            }
            name->onSubmit = [this, index](const String& text)
            {
                this->commitMemberName(index, text);
            };
            line->attachChild(name);

            attachSchemaTypeDrop(
                *line,
                mDoc->schemaMemberTypeName(index),
                i,
                mTypeIndex,
                0.0f,
                0.0f,
                memberTypeW,
                fieldH,
                mHost.scale,
                fontPath,
                smallPx,
                ink,
                mute,
                t,
                *mDoc,
                mHost,
                mOverlay,
                mTypeMenu,
                onPickType,
                [openTypeMenu, i]() { openTypeMenu(i); });

            auto up = makeButton("Up", fontPath, smallPx, btnW, fieldH, quiet, ink);
            up->onClick = [this, i]()
            {
                if (mDoc == nullptr || i <= 0)
                {
                    return;
                }
                mDoc->moveSchemaMember(static_cast<u32_t>(i), -1);
                this->notifyRebuild();
            };
            auto down = makeButton("Down", fontPath, smallPx, btnW, fieldH, quiet, ink);
            down->onClick = [this, i]()
            {
                if (mDoc == nullptr || i + 1 >= static_cast<int>(mDoc->schemaMemberCount()))
                {
                    return;
                }
                mDoc->moveSchemaMember(static_cast<u32_t>(i), 1);
                this->notifyRebuild();
            };
            auto remove = makeDangerButton("Del", fontPath, smallPx, btnW, fieldH);
            remove->onClick = [this, i]()
            {
                if (mDoc == nullptr)
                {
                    return;
                }
                mDoc->deleteSchemaMember(static_cast<u32_t>(i));
                this->notifyRebuild();
            };
            line->attachChild(up);
            line->attachChild(down);
            line->attachChild(remove);
            body->attachChild(line);
        }
        auto add = makeButton("Add Member", fontPath, smallPx, 96.0f * s, fieldH, t.ghost, ink);
        add->onClick = [this]()
        {
            if (mDoc == nullptr || !mDoc->addSchemaMember())
            {
                return;
            }
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

        if (mTypeIndex >= 0 && mTypeMenu.anchor() != nullptr && mOverlay != nullptr && mTypeMenu.menu() == nullptr)
        {
            mTypeMenu.open(*mTypeMenu.anchor(), mTypeIndex, *mDoc, mHost, mHost.scale, mOverlay, onPickType);
        }
    }

}