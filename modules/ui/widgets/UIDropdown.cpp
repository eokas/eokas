#include "UIDropdown.h"
#include "../UIFont.h"
#include "../UIStroke.h"
#include <cmath>

namespace eokas
{
    namespace
    {
        float snap(float v)
        {
            return floorf(v + 0.5f);
        }

        float lineHeight(const UIText* text)
        {
            float textH = UIText::kDefaultFontSize;
            if (text == nullptr)
            {
                return textH;
            }
            if (text->style.fontSize > 0.0f)
            {
                textH = text->style.fontSize;
            }
            UIFont* font = UIFont::find(text->style.fontPath);
            if (font != nullptr && font->isOpen())
            {
                float bake = (float)font->pixelSize();
                float scale = (text->style.fontSize > 0.0f ? text->style.fontSize : bake) / bake;
                textH = (font->ascender() - font->descender()) * scale;
            }
            return textH;
        }

        void placeLabel(UIWidget& parent, UIText* text, const Vector2& areaSize, float paddingX, float paddingY)
        {
            if (text == nullptr)
            {
                return;
            }
            float height = snap(lineHeight(text));
            float width = areaSize.x;
            float x = snap(paddingX);
            float innerH = areaSize.y - paddingY * 2.0f;
            if (innerH < 0.0f)
            {
                innerH = 0.0f;
            }
            float y = snap(paddingY + (innerH - height) * 0.5f);
            parent.placeChild(*text, Vector2(x, y), Vector2(width, height));
        }
    }

    void UIDropdownItem::setLabel(const String& text)
    {
        if (!mLabel)
        {
            mLabel = std::make_shared<UIText>();
            this->detachChildren();
            this->attachChild(mLabel);
        }
        mLabel->text = text;
    }

    UIText* UIDropdownItem::label() const
    {
        return mLabel.get();
    }

    void UIDropdownItem::render(UIPrimitive& primitive)
    {
        if (!visible)
        {
            return;
        }
        placeLabel(*this, mLabel.get(), shape.size, paddingX, paddingY);
        Matrix3 world = worldTrans();
        Color bg = background;
        if (pressed)
        {
            bg = pressedFill;
        }
        else if (hovered)
        {
            bg = hoverFill;
        }
        else if (selected)
        {
            bg = selectedFill;
        }
        primitive.addQuad(world, Rect(0.0f, 0.0f, shape.size.x, shape.size.y), UIFont::solidUV(), bg);
        UIWidget::render(primitive);
    }

    void UIDropdownItem::triggerPointerRelease()
    {
        const bool activate = pressed;
        UIWidget::triggerPointerRelease();
        if (activate && owner != nullptr)
        {
            owner->setValue(index);
            owner->setExpanded(false);
        }
    }

    UIDropdown::UIDropdown()
    {
        mCaption = std::make_shared<UIText>();
        this->attachChild(mCaption);
        auto panel = std::make_shared<UIWidget>();
        panel->floating = true;
        panel->visible = false;
        panel->pickable = true;
        dropdown = panel.get();
        this->attachChild(panel);
        this->syncCaption();
        onGotFocus = []() {};
        onLostFocus = [this]() { this->setExpanded(false); };
    }

    UIDropdown::~UIDropdown()
    {
        for (auto& item : mItems)
        {
            if (item)
            {
                item->owner = nullptr;
            }
        }
    }

    UIDropdownItem* UIDropdown::findItem(int index) const
    {
        for (const auto& item : mItems)
        {
            if (item && item->index == index)
            {
                return item.get();
            }
        }
        return nullptr;
    }

    void UIDropdown::addItem(int index, const String& label)
    {
        if (index == -1)
        {
            return;
        }
        if (UIDropdownItem* existing = this->findItem(index))
        {
            existing->setLabel(label);
            this->copyFont(existing->label());
            this->syncCaption();
            return;
        }
        auto item = std::make_shared<UIDropdownItem>();
        item->index = index;
        item->owner = this;
        item->setLabel(label);
        this->copyFont(item->label());
        item->visible = false;
        mItems.push_back(item);
        if (dropdown)
        {
            dropdown->attachChild(item);
        }
        this->syncCaption();
    }

    void UIDropdown::removeItem(int index)
    {
        auto iter = mItems.begin();
        for (; iter != mItems.end(); ++iter)
        {
            if (*iter && (*iter)->index == index)
            {
                break;
            }
        }
        if (iter == mItems.end())
        {
            return;
        }
        UIDropdownItem* raw = iter->get();
        raw->owner = nullptr;
        mItems.erase(iter);
        if (dropdown)
        {
            dropdown->detachChild(raw);
        }
        if (value == index)
        {
            this->setValue(-1);
        }
    }

    void UIDropdown::clearItems()
    {
        for (auto& item : mItems)
        {
            if (item)
            {
                item->owner = nullptr;
            }
        }
        mItems.clear();
        if (dropdown)
        {
            dropdown->detachChildren();
        }
        this->setExpanded(false);
        if (value != -1)
        {
            this->setValue(-1);
            return;
        }
        this->syncCaption();
    }

    void UIDropdown::setValue(int index)
    {
        if (index == value)
        {
            return;
        }
        if (index != -1 && this->findItem(index) == nullptr)
        {
            return;
        }
        value = index;
        this->syncCaption();
        this->syncPopup();
        if (onValueChanged)
        {
            onValueChanged(value);
        }
    }

    void UIDropdown::setExpanded(bool next)
    {
        if (expanded == next)
        {
            return;
        }
        expanded = next;
        this->syncPopup();
    }

    String UIDropdown::labelOf(int index) const
    {
        UIDropdownItem* item = this->findItem(index);
        if (item == nullptr || item->label() == nullptr)
        {
            return String();
        }
        return item->label()->text;
    }

    UIText* UIDropdown::caption() const
    {
        return mCaption.get();
    }

    void UIDropdown::copyFont(UIText* text) const
    {
        if (mCaption == nullptr || text == nullptr)
        {
            return;
        }
        text->style.fontPath = mCaption->style.fontPath;
        text->style.fontSize = mCaption->style.fontSize;
        text->style.color = this->text.color;
    }

    void UIDropdown::syncCaption()
    {
        if (!mCaption)
        {
            return;
        }
        UIDropdownItem* item = this->findItem(value);
        if (item != nullptr && item->label() != nullptr)
        {
            mCaption->text = item->label()->text;
            mCaption->style.color = this->text.color;
            return;
        }
        mCaption->text = placeholder;
        mCaption->style.color = placeholderStyle.color;
    }

    void UIDropdown::syncPopup()
    {
        if (dropdown == nullptr)
        {
            return;
        }
        float rowH = shape.size.y;
        if (rowH < 1.0f)
        {
            rowH = 1.0f;
        }
        dropdown->visible = expanded && visible;
        dropdown->pickable = pickable;
        float panelY = snap(shape.size.y);
        float panelH = rowH * (float)mItems.size();
        this->placeChild(*dropdown, Vector2(0.0f, panelY), Vector2(shape.size.x, panelH));
        float y = 0.0f;
        for (auto& item : mItems)
        {
            if (!item)
            {
                continue;
            }
            item->visible = dropdown->visible;
            item->pickable = pickable;
            item->selected = item->index == value;
            item->paddingX = paddingX;
            item->paddingY = paddingY;
            item->background = popup;
            item->hoverFill = itemHover;
            item->pressedFill = itemPressed;
            item->selectedFill = itemSelected;
            this->copyFont(item->label());
            dropdown->placeChild(*item, Vector2(0.0f, snap(y)), Vector2(shape.size.x, rowH));
            y += rowH;
        }
    }

    int UIDropdown::neighbor(int direction) const
    {
        if (mItems.empty())
        {
            return value;
        }
        int pos = -1;
        for (size_t i = 0; i < mItems.size(); i++)
        {
            if (mItems[i] && mItems[i]->index == value)
            {
                pos = (int)i;
                break;
            }
        }
        if (pos < 0)
        {
            return direction < 0 ? mItems.back()->index : mItems.front()->index;
        }
        int next = pos + direction;
        if (next < 0)
        {
            next = 0;
        }
        if (next >= (int)mItems.size())
        {
            next = (int)mItems.size() - 1;
        }
        return mItems[(size_t)next]->index;
    }

    void UIDropdown::layoutCaption()
    {
        placeLabel(*this, mCaption.get(), shape.size, paddingX, paddingY);
    }

    void UIDropdown::drawBorder(UIPrimitive& primitive, const Matrix3& world, const Rect& area) const
    {
        UIStroke::border(primitive, world, area, border);
    }

    void UIDropdown::drawChevron(UIPrimitive& primitive, const Matrix3& world) const
    {
        float s = snap(Math::min_s(shape.size.y * 0.28f, 8.0f));
        if (s < 4.0f)
        {
            s = 4.0f;
        }
        float cx = snap(shape.size.x - paddingX - s * 0.5f);
        float cy = snap(shape.size.y * 0.5f);
        Rect uv = UIFont::solidUV();
        if (expanded)
        {
            primitive.addQuad(
                UIShape::transformPoint(world, Vector2(cx - s * 0.5f, cy + s * 0.25f)),
                UIShape::transformPoint(world, Vector2(cx + s * 0.5f, cy + s * 0.25f)),
                UIShape::transformPoint(world, Vector2(cx, cy - s * 0.35f)),
                UIShape::transformPoint(world, Vector2(cx, cy - s * 0.35f)),
                uv,
                chevron);
            return;
        }
        primitive.addQuad(
            UIShape::transformPoint(world, Vector2(cx - s * 0.5f, cy - s * 0.25f)),
            UIShape::transformPoint(world, Vector2(cx + s * 0.5f, cy - s * 0.25f)),
            UIShape::transformPoint(world, Vector2(cx, cy + s * 0.35f)),
            UIShape::transformPoint(world, Vector2(cx, cy + s * 0.35f)),
            uv,
            chevron);
    }

    void UIDropdown::render(UIPrimitive& primitive)
    {
        if (!visible)
        {
            return;
        }
        this->syncCaption();
        this->layoutCaption();
        this->syncPopup();
        Matrix3 world = worldTrans();
        Color bg = background;
        if (pickable && pressed)
        {
            bg = pressedFill;
        }
        else if (pickable && (hovered || expanded))
        {
            bg = hoverFill;
        }
        primitive.addQuad(world, Rect(0.0f, 0.0f, shape.size.x, shape.size.y), UIFont::solidUV(), bg);
        this->drawBorder(primitive, world, Rect(0.0f, 0.0f, shape.size.x, shape.size.y));
        this->drawChevron(primitive, world);
        UIWidget::render(primitive);
    }

    void UIDropdown::triggerPointerRelease()
    {
        if (pressed && pickable)
        {
            this->setExpanded(!expanded);
        }
        UIWidget::triggerPointerRelease();
    }

    void UIDropdown::triggerKeyPress(const UIKey& key, const UIKeyMods& mods)
    {
        (void)mods;
        if (!pickable)
        {
            UIWidget::triggerKeyPress(key, mods);
            return;
        }
        if (key == UIKey::Enter)
        {
            this->setExpanded(!expanded);
            UIWidget::triggerKeyPress(key, mods);
            return;
        }
        if (key == UIKey::Up || key == UIKey::Down)
        {
            int next = this->neighbor(key == UIKey::Up ? -1 : 1);
            this->setValue(next);
        }
        UIWidget::triggerKeyPress(key, mods);
    }
}
