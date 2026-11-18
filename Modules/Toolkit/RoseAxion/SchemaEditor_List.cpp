#include "./SchemaEditor_List.h"
#include "./Theme.h"
#include "./UiKit.h"

#include "UI/Main.h"

namespace eokas::RoseAxion {

    void RoseSchemaEditor_List::bind(TableDocument& doc, const Host& host)
    {
        mDoc = &doc;
        mHost = host;
    }

    void RoseSchemaEditor_List::setOverlay(UIWidget* overlay)
    {
        if (mOverlay == overlay)
        {
            return;
        }
        this->detachTypeMenu();
        mOverlay = overlay;
        mHost.overlay = overlay;
    }

    void RoseSchemaEditor_List::detachTypeMenu()
    {
        mTypeMenu.close(mOverlay);
    }

    const String& RoseSchemaEditor_List::font() const
    {
        return mHost.font;
    }

    float RoseSchemaEditor_List::scale() const
    {
        return hostScale(mHost.scale);
    }

    void RoseSchemaEditor_List::notifyRebuild()
    {
        if (mHost.requestRebuild)
        {
            mHost.requestRebuild();
        }
    }

    void RoseSchemaEditor_List::reset()
    {
        mTypeIndex = -2;
        mKeepY = 0.0f;
        mView = nullptr;
        mFocus = nullptr;
        this->detachTypeMenu();
        this->detachChildren();
    }

    float RoseSchemaEditor_List::scrollY() const
    {
        return mView != nullptr ? mView->scrollY() : mKeepY;
    }

    void RoseSchemaEditor_List::setScroll(float y)
    {
        mKeepY = y;
        if (mView != nullptr)
        {
            mView->setScroll(0.0f, y);
        }
    }

    void RoseSchemaEditor_List::layout()
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

    void RoseSchemaEditor_List::placeOverlayEditors()
    {
        mTypeMenu.place(mOverlay);
    }

    void RoseSchemaEditor_List::applyElementType(const String& typeName)
    {
        if (mDoc == nullptr)
        {
            return;
        }
        mTypeIndex = -2;
        this->detachTypeMenu();
        mDoc->setSchemaElementType(typeName);
        this->notifyRebuild();
    }

    bool RoseSchemaEditor_List::handleEscape()
    {
        if (mTypeMenu.visible() || mTypeIndex == -1)
        {
            mTypeIndex = -2;
            this->detachTypeMenu();
            this->notifyRebuild();
            return true;
        }
        return false;
    }

    void RoseSchemaEditor_List::rebuild()
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
        const float rowH = 36.0f * s;
        const float typeColW = 180.0f * s;
        const RoseTheme& t = theme();
        const Color paper = t.paper;
        const Color ink = t.ink;
        const Color mute = t.mute;
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

        auto title = makeText("Element Type", fontPath, smallPx, mute, innerW, 18.0f * s);
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

        const int elementSlot = -1;
        auto onPickType = [this](const String& name) { this->applyElementType(name); };
        auto openTypeMenu = [this]() { mTypeIndex = -1; };

        attachSchemaTypeDrop(
            *row,
            element,
            elementSlot,
            mTypeIndex,
            0.0f,
            (rowH - fieldH) * 0.5f,
            typeColW,
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
            openTypeMenu);
        body->attachChild(row);

        body->layout();
        setBox(bodyCard, Rect(0.0f, 0.0f, cardW, body->shape.size.y));
        bodyCard->attachChild(body);
        stack->attachChild(bodyCard);
        stack->layout();
        view->addChild(stack);
        view->setScroll(0.0f, mKeepY);
        this->attachChild(view);
        mView = view.get();

        if (mTypeIndex == elementSlot && mTypeMenu.anchor() != nullptr && mOverlay != nullptr && mTypeMenu.menu() == nullptr)
        {
            mTypeMenu.open(*mTypeMenu.anchor(), elementSlot, *mDoc, mHost, mHost.scale, mOverlay, onPickType);
        }
    }

}