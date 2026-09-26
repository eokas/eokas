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

        void placeLabel(UIText* text, const Rect& area, float paddingX, float paddingY)
        {
            if (text == nullptr)
            {
                return;
            }
            float height = snap(lineHeight(text));
            float width = area.size.x;
            float x = snap(area.origin.x + paddingX);
            float innerH = area.size.y - paddingY * 2.0f;
            if (innerH < 0.0f)
            {
                innerH = 0.0f;
            }
            float y = snap(area.origin.y + paddingY + (innerH - height) * 0.5f);
            text->layout(Rect(x, y, width, height));
        }
    }

    void UIDropdownItem::setLabel(const String& text)
    {
        if (!mLabel)
        {
            mLabel = std::make_shared<UIText>();
            children.clear();
            children.push_back(mLabel);
        }
        mLabel->text = text;
    }

    UIText* UIDropdownItem::label() const
    {
        return mLabel.get();
    }

    void UIDropdownItem::layout(const Rect& rect)
    {
        this->rect = rect;
        placeLabel(mLabel.get(), Rect(Vector2::ZERO, rect.size), paddingX, paddingY);
    }

    void UIDropdownItem::render(UIPrimitive& primitive)
    {
        if (!visible)
        {
            return;
        }
        primitive.pushScaleAround(rect.origin, localScale);
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
        primitive.addQuad(rect, UIFont::solidUV(), bg);
        primitive.popOrigin();
        UIWidget::render(primitive);
    }

    void UIDropdownItem::triggerClick()
    {
        UIWidget::triggerClick();
        if (owner != nullptr)
        {
            owner->setValue(index);
            owner->setExpanded(false);
        }
    }

    UIDropdown::UIDropdown()
    {
        mCaption = std::make_shared<UIText>();
        children.push_back(mCaption);
        auto panel = std::make_shared<UIWidget>();
        panel->floating = true;
        panel->visible = false;
        panel->pickable = true;
        dropdown = panel.get();
        children.push_back(panel);
        this->syncCaption();
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
            dropdown->children.push_back(item);
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
            for (auto child = dropdown->children.begin(); child != dropdown->children.end(); ++child)
            {
                if (child->get() == raw)
                {
                    dropdown->children.erase(child);
                    break;
                }
            }
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
            dropdown->children.clear();
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
        float rowH = rect.size.y;
        if (rowH < 1.0f)
        {
            rowH = 1.0f;
        }
        float y = 0.0f;
        dropdown->visible = expanded && visible;
        dropdown->pickable = pickable;
        float panelY = snap(rect.size.y);
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
            item->layout(Rect(Vector2(0.0f, snap(y)), Vector2(rect.size.x, rowH)));
            y += rowH;
        }
        dropdown->layout(Rect(Vector2(0.0f, panelY), Vector2(rect.size.x, rowH * (float)mItems.size())));
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
        placeLabel(mCaption.get(), Rect(Vector2::ZERO, rect.size), paddingX, paddingY);
    }

    void UIDropdown::drawBorder(UIPrimitive& primitive, const Rect& area) const
    {
        UIStroke::border(primitive, area, border);
    }

    void UIDropdown::drawChevron(UIPrimitive& primitive) const
    {
        float s = snap(Math::min_s(rect.size.y * 0.28f, 8.0f));
        if (s < 4.0f)
        {
            s = 4.0f;
        }
        float cx = snap(rect.origin.x + rect.size.x - paddingX - s * 0.5f);
        float cy = snap(rect.origin.y + rect.size.y * 0.5f);
        Rect uv = UIFont::solidUV();
        if (expanded)
        {
            primitive.addQuad(
                Vector2(cx - s * 0.5f, cy + s * 0.25f),
                Vector2(cx + s * 0.5f, cy + s * 0.25f),
                Vector2(cx, cy - s * 0.35f),
                Vector2(cx, cy - s * 0.35f),
                uv,
                chevron);
            return;
        }
        primitive.addQuad(
            Vector2(cx - s * 0.5f, cy - s * 0.25f),
            Vector2(cx + s * 0.5f, cy - s * 0.25f),
            Vector2(cx, cy + s * 0.35f),
            Vector2(cx, cy + s * 0.35f),
            uv,
            chevron);
    }

    void UIDropdown::layout(const Rect& rect)
    {
        this->rect = rect;
        this->syncCaption();
        this->layoutCaption();
        this->syncPopup();
    }

    void UIDropdown::render(UIPrimitive& primitive)
    {
        if (!visible)
        {
            return;
        }
        primitive.pushScaleAround(rect.origin, localScale);
        Color bg = background;
        if (pickable && pressed)
        {
            bg = pressedFill;
        }
        else if (pickable && (hovered || expanded))
        {
            bg = hoverFill;
        }
        primitive.addQuad(rect, UIFont::solidUV(), bg);
        this->drawBorder(primitive, rect);
        this->drawChevron(primitive);
        primitive.popOrigin();
        UIWidget::render(primitive);
    }

    void UIDropdown::triggerClick()
    {
        if (pickable)
        {
            this->setExpanded(!expanded);
        }
        UIWidget::triggerClick();
    }

    void UIDropdown::triggerBlur()
    {
        this->setExpanded(false);
        UIWidget::triggerBlur();
    }

    void UIDropdown::triggerKey(UIKey key, const UIKeyMods& mods)
    {
        (void)mods;
        if (!pickable)
        {
            return;
        }
        if (key == UIKey::Enter)
        {
            this->setExpanded(!expanded);
            return;
        }
        if (key == UIKey::Up || key == UIKey::Down)
        {
            int next = this->neighbor(key == UIKey::Up ? -1 : 1);
            this->setValue(next);
        }
    }
}
