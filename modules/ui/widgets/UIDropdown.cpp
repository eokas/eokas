#include "UIDropdown.h"
#include "../UIFont.h"
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
            if (text->fontSize > 0.0f)
            {
                textH = text->fontSize;
            }
            if (text->font != nullptr && text->font->isOpen())
            {
                float bake = (float)text->font->pixelSize();
                float scale = (text->fontSize > 0.0f ? text->fontSize : bake) / bake;
                textH = (text->font->ascender() - text->font->descender()) * scale;
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
            float width = area.width;
            float x = snap(area.x + paddingX);
            float innerH = area.height - paddingY * 2.0f;
            if (innerH < 0.0f)
            {
                innerH = 0.0f;
            }
            float y = snap(area.y + paddingY + (innerH - height) * 0.5f);
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
        placeLabel(mLabel.get(), rect, paddingX, paddingY);
    }

    void UIDropdownItem::render(UIShape& shape)
    {
        if (!visible)
        {
            return;
        }
        Color bg = background;
        if (pressed)
        {
            bg = pressedColor;
        }
        else if (hovered)
        {
            bg = hoverColor;
        }
        else if (selected)
        {
            bg = selectedColor;
        }
        shape.addQuad(rect, UIFont::solidUV(), bg);
        if (mLabel)
        {
            mLabel->render(shape);
        }
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
        panel->interactive = true;
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
        text->fontPath = mCaption->fontPath;
        text->fontSize = mCaption->fontSize;
        if (mCaption->font != nullptr)
        {
            text->font = mCaption->font;
        }
        text->color = textColor;
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
            mCaption->color = textColor;
            return;
        }
        mCaption->text = placeholder;
        mCaption->color = placeholderColor;
    }

    void UIDropdown::syncPopup()
    {
        if (dropdown == nullptr)
        {
            return;
        }
        float rowH = rect.height;
        if (rowH < 1.0f)
        {
            rowH = 1.0f;
        }
        float y = rect.y + rect.height;
        dropdown->visible = expanded && visible;
        dropdown->interactive = interactive;
        float panelY = snap(y);
        for (auto& item : mItems)
        {
            if (!item)
            {
                continue;
            }
            item->visible = dropdown->visible;
            item->interactive = interactive;
            item->selected = item->index == value;
            item->paddingX = paddingX;
            item->paddingY = paddingY;
            item->background = popupColor;
            item->hoverColor = itemHoverColor;
            item->pressedColor = itemPressedColor;
            item->selectedColor = itemSelectedColor;
            this->copyFont(item->label());
            item->layout(Rect(rect.x, snap(y), rect.width, rowH));
            y += rowH;
        }
        dropdown->layout(Rect(rect.x, panelY, rect.width, rowH * (float)mItems.size()));
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
        placeLabel(mCaption.get(), rect, paddingX, paddingY);
    }

    void UIDropdown::drawBorder(UIShape& shape, const Rect& area) const
    {
        float t = borderThickness;
        if (t <= 0.0f || area.width <= 0.0f || area.height <= 0.0f)
        {
            return;
        }
        Rect uv = UIFont::solidUV();
        shape.addQuad(Rect(area.x, area.y, area.width, t), uv, borderColor);
        shape.addQuad(Rect(area.x, area.y + area.height - t, area.width, t), uv, borderColor);
        shape.addQuad(Rect(area.x, area.y, t, area.height), uv, borderColor);
        shape.addQuad(Rect(area.x + area.width - t, area.y, t, area.height), uv, borderColor);
    }

    void UIDropdown::drawChevron(UIShape& shape) const
    {
        float s = snap(Math::min_s(rect.height * 0.28f, 8.0f));
        if (s < 4.0f)
        {
            s = 4.0f;
        }
        float cx = snap(rect.x + rect.width - paddingX - s * 0.5f);
        float cy = snap(rect.y + rect.height * 0.5f);
        Rect uv = UIFont::solidUV();
        if (expanded)
        {
            shape.addQuad(
                Vector2(cx - s * 0.5f, cy + s * 0.25f),
                Vector2(cx + s * 0.5f, cy + s * 0.25f),
                Vector2(cx, cy - s * 0.35f),
                Vector2(cx, cy - s * 0.35f),
                uv,
                chevronColor);
            return;
        }
        shape.addQuad(
            Vector2(cx - s * 0.5f, cy - s * 0.25f),
            Vector2(cx + s * 0.5f, cy - s * 0.25f),
            Vector2(cx, cy + s * 0.35f),
            Vector2(cx, cy + s * 0.35f),
            uv,
            chevronColor);
    }

    void UIDropdown::layout(const Rect& rect)
    {
        this->rect = rect;
        this->syncCaption();
        this->layoutCaption();
        this->syncPopup();
    }

    void UIDropdown::render(UIShape& shape)
    {
        if (!visible)
        {
            return;
        }
        Color bg = background;
        if (interactive && pressed)
        {
            bg = pressedColor;
        }
        else if (interactive && (hovered || expanded))
        {
            bg = hoverColor;
        }
        shape.addQuad(rect, UIFont::solidUV(), bg);
        this->drawBorder(shape, rect);
        this->drawChevron(shape);
        UIWidget::render(shape);
    }

    void UIDropdown::triggerClick()
    {
        if (interactive)
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
        if (!interactive)
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
