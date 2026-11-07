#include "UIMenu.h"
#include "../UIFont.h"
#include <cmath>

namespace eokas
{
    namespace
    {
        constexpr int kChipSegments = 16;

        float chipLeading(const UIMenuItem& item)
        {
            if (!item.showChip)
            {
                return 0.0f;
            }
            float size = item.chipSize;
            if (size < 1.0f)
            {
                size = 1.0f;
            }
            return size + item.chipGap;
        }

        void addDisc(const UIWidget& widget, UIPrimitive& primitive, float cx, float cy, float radius, const Color& color)
        {
            if (radius <= 0.0f || color.a <= 0.0f)
            {
                return;
            }
            Rect uv = UIFont::solidUV();
            Vector2 center(cx, cy);
            for (int i = 0; i < kChipSegments; i++)
            {
                float a0 = Math::PI_MUL_2 * ((float)i / (float)kChipSegments);
                float a1 = Math::PI_MUL_2 * ((float)(i + 1) / (float)kChipSegments);
                Vector2 e0(cx + cosf(a0) * radius, cy + sinf(a0) * radius);
                Vector2 e1(cx + cosf(a1) * radius, cy + sinf(a1) * radius);
                primitive.addQuad(
                    widget.localToScreen(center),
                    widget.localToScreen(e0),
                    widget.localToScreen(e1),
                    widget.localToScreen(center),
                    uv,
                    color);
            }
        }
    }

    UIMenuItem::UIMenuItem()
    {
        this->setContent(std::make_shared<UIText>());
    }

    void UIMenuItem::setContent(const UIWidget::Ref& widget)
    {
        content = widget;
        this->detachChildren();
        if (content)
        {
            this->attachChild(content);
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
        content->layout();
        float lead = chipLeading(*this);
        float height = content->shape.size.y;
        if (showChip && chipSize > height)
        {
            height = chipSize;
        }
        this->resize(Vector2(content->shape.size.x + paddingX * 2.0f + lead, height + paddingY * 2.0f));
    }

    void UIMenuItem::layout()
    {
        if (!visible)
        {
            return;
        }
        if (content)
        {
            float innerH = shape.size.y - paddingY * 2.0f;
            if (innerH < 0.0f)
            {
                innerH = 0.0f;
            }
            float y = paddingY;
            if (content->shape.size.y < innerH)
            {
                y = paddingY + (innerH - content->shape.size.y) * 0.5f;
            }
            Vector2 pos(floorf(paddingX + chipLeading(*this) + 0.5f), floorf(y + 0.5f));
            this->placeChild(*content, pos);
        }
    }

    void UIMenuItem::render(UIPrimitive& primitive)
    {
        if (!visible)
        {
            return;
        }
        Color bg = background;
        if (isHovered())
        {
            bg = hoverFill;
        }
        else if (selected)
        {
            bg = selectedFill;
        }
        Matrix3 localToScreen = matrixLocalToScreen();
        primitive.addQuad(localToScreen, Rect(0.0f, 0.0f, shape.size.x, shape.size.y), UIFont::solidUV(), bg);
        if (showChip)
        {
            float size = chipSize;
            if (size < 1.0f)
            {
                size = 1.0f;
            }
            float x = floorf(paddingX + 0.5f);
            float y = floorf((shape.size.y - size) * 0.5f + 0.5f);
            float radius = size * 0.5f;
            float cx = x + radius;
            float cy = y + radius;
            Color line(0.040f, 0.043f, 0.055f, 1.0f);
            addDisc(*this, primitive, cx, cy, radius, line);
            float border = 1.0f;
            if (border * 2.0f >= size)
            {
                border = 0.0f;
            }
            float inner = radius - border;
            if (inner > 0.0f)
            {
                addDisc(*this, primitive, cx, cy, inner, chip);
            }
        }
        UIWidget::render(primitive);
    }

    UIMenu::UIMenu()
    {
        color = Color(1.0f, 1.0f, 1.0f, 1.0f);
        list = std::make_shared<UIList>();
        list->direction = direction;
        list->padding = padding;
        list->spacing = spacing;
        list->color = Color(0.0f, 0.0f, 0.0f, 0.0f);
        this->attachChild(list);
    }

    void UIMenu::addItem(const std::shared_ptr<UIMenuItem>& item)
    {
        if (list && item)
        {
            list->attachChild(item);
        }
    }

    void UIMenu::layout()
    {
        if (!visible)
        {
            return;
        }
        if (list)
        {
            list->direction = direction;
            list->padding = padding;
            list->spacing = spacing;

            float maxItemWidth = 0.0f;
            float maxItemHeight = 0.0f;
            float sumItemHeight = 0.0f;
            int itemCount = 0;
            for (auto& child : list->children())
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
                this->resize(Vector2(shape.size.x, padding * 2.0f + maxItemHeight));
            }
            else
            {
                this->resize(Vector2(padding * 2.0f + maxItemWidth, padding * 2.0f + sumItemHeight + gap));
            }

            for (auto& child : list->children())
            {
                UIMenuItem* item = dynamic_cast<UIMenuItem*>(child.get());
                if (item == nullptr || !item->visible)
                {
                    continue;
                }
                if (direction == UIDirection::Horizontal)
                {
                    item->resize(Vector2(item->shape.size.x, maxItemHeight));
                }
                else
                {
                    item->resize(Vector2(maxItemWidth, item->shape.size.y));
                }
            }

            this->placeChild(*list, Vector2::ZERO, shape.size);
        }
        UIWidget::layout();
    }

    bool UIMenu::handlePointerPressAnywhere(const UIPointerEvent& event)
    {
        if (overlay && visible)
        {
            const Vector2 screen = event.position;
            bool inside = this->pick(this->screenToParentPivot(screen)) != nullptr;
            if (!inside && host != nullptr)
            {
                inside = host->pick(host->screenToParentPivot(screen)) != nullptr;
            }
            if (!inside)
            {
                visible = false;
            }
        }
        return UIWidget::handlePointerPressAnywhere(event);
    }

    void UIMenu::render(UIPrimitive& primitive)
    {
        if (!visible)
        {
            return;
        }
        UIWidget::render(primitive);
    }
}
