#include "UIMenu.h"
#include "../UIFont.h"
#include <cmath>

namespace eokas
{
    UIMenuItem::UIMenuItem()
    {
        this->setContent(std::make_shared<UIText>());
    }

    void UIMenuItem::setContent(const std::shared_ptr<UIWidget>& widget)
    {
        content = widget;
        children.clear();
        if (content)
        {
            children.push_back(content);
        }
    }

    void UIMenuItem::setText(const String& text)
    {
        UIText* t = this->label();
        if (t == nullptr)
        {
            auto created = std::make_shared<UIText>();
            this->setContent(created);
            t = created.get();
        }
        t->text = text;
    }

    UIText* UIMenuItem::label() const
    {
        return dynamic_cast<UIText*>(content.get());
    }

    void UIMenuItem::syncSize()
    {
        if (!content)
        {
            return;
        }

        if (UIText* t = this->label())
        {
            UIFont* font = UIFont::find(t->style.fontPath);
            if (font != nullptr && font->isOpen())
            {
                float bake = (float)font->pixelSize();
                float scale = (t->style.fontSize > 0.0f ? t->style.fontSize : bake) / bake;
                float width = 0.0f;
                for (size_t i = 0; i < t->text.length(); i++)
                {
                    width += font->glyph(t->text.at(i)).advance * scale;
                }
                t->shape.origin.x += t->shape.pivot.x * ((floorf(width + 0.5f)) - t->shape.size.x);
                t->shape.size.x = floorf(width + 0.5f);
                float tight = font->ascender() - font->descender();
                t->shape.origin.y += t->shape.pivot.y * ((floorf(tight * scale + 0.5f)) - t->shape.size.y);
                t->shape.size.y = floorf(tight * scale + 0.5f);
            }
            shape.origin.x += shape.pivot.x * ((t->shape.size.x + paddingX * 2.0f) - shape.size.x);
            shape.size.x = t->shape.size.x + paddingX * 2.0f;
            shape.origin.y += shape.pivot.y * ((t->shape.size.y + paddingY * 2.0f) - shape.size.y);
            shape.size.y = t->shape.size.y + paddingY * 2.0f;
            return;
        }

        shape.origin.x += shape.pivot.x * ((content->shape.size.x + paddingX * 2.0f) - shape.size.x);
        shape.size.x = content->shape.size.x + paddingX * 2.0f;
        shape.origin.y += shape.pivot.y * ((content->shape.size.y + paddingY * 2.0f) - shape.size.y);
        shape.size.y = content->shape.size.y + paddingY * 2.0f;
    }

    void UIMenuItem::layout(const Rect& rect)
    {
        {
            Rect _box = rect;
            shape.size = _box.size;
            shape.origin = _box.origin + shape.pivot * shape.size;
        }
        if (!content)
        {
            return;
        }
        Vector2 pos(floorf(paddingX + 0.5f), floorf(paddingY + 0.5f));
        content->layout(Rect(pos, content->shape.size));
    }

    void UIMenuItem::render(UIPrimitive& primitive)
    {
        if (!visible)
        {
            return;
        }

        primitive.pushScaleAround(shape.origin, shape.scale);
        Color bg = hovered ? hoverFill : background;
        primitive.addQuad(Rect(shape.left(), shape.top(), shape.size.x, shape.size.y), UIFont::solidUV(), bg);
        primitive.popOrigin();
        UIWidget::render(primitive);
    }

    UIMenu::UIMenu()
    {
        list = std::make_shared<UIList>();
        list->direction = direction;
        list->padding = padding;
        list->spacing = spacing;
        list->color = Color(0.0f, 0.0f, 0.0f, 0.0f);
        children.push_back(list);
    }

    void UIMenu::addItem(const std::shared_ptr<UIMenuItem>& item)
    {
        if (list && item)
        {
            list->addChild(item);
        }
    }

    void UIMenu::layout(const Rect& rect)
    {
        {
            Rect _box = rect;
            shape.size = _box.size;
            shape.origin = _box.origin + shape.pivot * shape.size;
        }
        if (!list)
        {
            return;
        }

        list->direction = direction;
        list->padding = padding;
        list->spacing = spacing;

        float maxItemWidth = 0.0f;
        float maxItemHeight = 0.0f;
        float sumItemHeight = 0.0f;
        int itemCount = 0;
        for (auto& child : list->children)
        {
            UIMenuItem* item = dynamic_cast<UIMenuItem*>(child.get());
            if (item == nullptr || !item->visible)
            {
                continue;
            }
            item->syncSize();
            if (item->shape.size.x > maxItemWidth)
            {
                maxItemWidth = item->shape.size.x;
            }
            if (item->shape.size.y > maxItemHeight)
            {
                maxItemHeight = item->shape.size.y;
            }
            sumItemHeight += item->shape.size.y;
            itemCount += 1;
        }

        float gap = itemCount > 1 ? spacing * (float)(itemCount - 1) : 0.0f;
        if (direction == UIDirection::Horizontal)
        {
            shape.origin.y += shape.pivot.y * ((padding * 2.0f + maxItemHeight) - shape.size.y);
            shape.size.y = padding * 2.0f + maxItemHeight;
        }
        else
        {
            shape.origin.x += shape.pivot.x * ((padding * 2.0f + maxItemWidth) - shape.size.x);
            shape.size.x = padding * 2.0f + maxItemWidth;
            shape.origin.y += shape.pivot.y * ((padding * 2.0f + sumItemHeight + gap) - shape.size.y);
            shape.size.y = padding * 2.0f + sumItemHeight + gap;
        }

        for (auto& child : list->children)
        {
            UIMenuItem* item = dynamic_cast<UIMenuItem*>(child.get());
            if (item == nullptr || !item->visible)
            {
                continue;
            }
            if (direction == UIDirection::Horizontal)
            {
                item->shape.origin.y += item->shape.pivot.y * ((maxItemHeight) - item->shape.size.y);
                item->shape.size.y = maxItemHeight;
            }
            else
            {
                item->shape.origin.x += item->shape.pivot.x * ((maxItemWidth) - item->shape.size.x);
                item->shape.size.x = maxItemWidth;
            }
        }

        list->layout(Rect(Vector2::ZERO, shape.size));
    }

    void UIMenu::render(UIPrimitive& primitive)
    {
        if (!visible)
        {
            return;
        }

        primitive.pushScaleAround(shape.origin, shape.scale);
        if (color.a > 0.0f)
        {
            primitive.addQuad(Rect(shape.left(), shape.top(), shape.size.x, shape.size.y), UIFont::solidUV(), color);
        }
        primitive.popOrigin();
        UIWidget::render(primitive);
    }
}
