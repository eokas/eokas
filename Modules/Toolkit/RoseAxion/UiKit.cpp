#include "./UiKit.h"
#include "./FieldTypes.h"

#include "UI/UIFont.h"

#include <chrono>
#include <cmath>

namespace eokas::RoseAxion {

    void setBox(UIWidget& widget, const Rect& box)
    {
        widget.shape.size = box.size;
        widget.shape.origin = box.origin + widget.shape.pivot * widget.shape.size;
    }

    void setBox(const std::shared_ptr<UIWidget>& widget, const Rect& box)
    {
        setBox(*widget, box);
    }

    void bindCommitOnFocusLoss(
        UIWidget& widget,
        const std::function<bool()>& isRebuilding,
        const std::function<void()>& commit)
    {
        widget.onFocusLoss = [isRebuilding, commit]()
        {
            if (isRebuilding && isRebuilding())
            {
                return;
            }
            if (commit)
            {
                commit();
            }
        };
    }

    void placeOnOverlay(UIWidget& overlay, UIWidget& widget, const Rect& screen, bool below)
    {
        Vector2 topLeft = overlay.screenToLocal(Vector2(screen.origin.x, below ? screen.origin.y + screen.size.y : screen.origin.y));
        if (!below)
        {
            widget.shape.setBox(topLeft, screen.size);
            return;
        }
        float w = widget.shape.size.x;
        if (w < screen.size.x)
        {
            w = screen.size.x;
        }
        widget.shape.setBox(topLeft, Vector2(w, widget.shape.size.y));
    }

    void placePopup(UIWidget& overlay, UIWidget& menu, const Rect& anchor)
    {
        menu.layout();
        Vector2 size = menu.shape.size;
        Rect limit = overlay.screenBounds();
        float x = anchor.origin.x;
        float y = anchor.origin.y + anchor.size.y;
        float limitRight = limit.origin.x + limit.size.x;
        float limitBottom = limit.origin.y + limit.size.y;
        if (x + size.x > limitRight)
        {
            x = limitRight - size.x;
        }
        if (x < limit.origin.x)
        {
            x = limit.origin.x;
        }
        if (y + size.y > limitBottom)
        {
            y = anchor.origin.y - size.y;
        }
        if (y < limit.origin.y)
        {
            y = limit.origin.y;
        }
        Vector2 local = overlay.screenToLocal(Vector2(x, y));
        menu.shape.setBox(local, size);
    }

    String fileTitleFromPath(const String& path)
    {
        if (path.isEmpty())
        {
            return "Untitled";
        }
        size_t slash = path.rfind('\\');
        size_t slash2 = path.rfind('/');
        size_t pos = String::npos;
        if (slash != String::npos)
        {
            pos = slash;
        }
        if (slash2 != String::npos && (pos == String::npos || slash2 > pos))
        {
            pos = slash2;
        }
        if (pos == String::npos)
        {
            return path;
        }
        return path.substr(pos + 1);
    }

    void styleText(UIText& text, const String& font, float size, const Color& color)
    {
        text.style.fontPath = font;
        text.style.fontSize = size;
        text.style.color = color;
    }

    void warmupRoseAxionFont(UIFont& font, float scale)
    {
        const float sizes[] = { kBodyPx, kTitleMdPx, kTitleLgPx };
        const String text = fontWarmupText();
        for (float px : sizes)
        {
            const float logical = px * scale;
            const uint32_t prepared = static_cast<uint32_t>(logical + 0.5f);
            (void)font.prepareSize(prepared);
            (void)font.measure(text, logical);
        }
    }

    std::shared_ptr<UIText> makeText(const String& value, const String& font, float size, const Color& color, float width, float height)
    {
        auto text = std::make_shared<UIText>();
        text->text = value;
        styleText(*text, font, size, color);
        text->maxWidth = width;
        setBox(text, Rect(0.0f, 0.0f, width, height));
        return text;
    }

    std::shared_ptr<UIButton> makeButton(const String& label, const String& font, float size, float minWidth, float minHeight, const Color& fill, const Color& ink)
    {
        auto button = std::make_shared<UIButton>();
        button->shape.pivot = Vector2(0.0f, 0.0f);
        float unit = size / 15.0f;
        if (unit < 0.5f)
        {
            unit = 1.0f;
        }
        button->paddingX = 12.0f * unit;
        button->paddingY = 6.0f * unit;
        button->background = fill;
        button->hoverFill = hoverOf(fill);
        button->pressedFill = pressOf(fill);
        button->setText(label);
        float width = minWidth;
        float height = minHeight;
        if (UIText* text = button->label())
        {
            styleText(*text, font, size, ink);
            text->layout();
            float tw = text->shape.size.x + button->paddingX * 2.0f;
            float th = text->shape.size.y + button->paddingY * 2.0f;
            if (tw > width)
            {
                width = tw;
            }
            if (th > height)
            {
                height = th;
            }
        }
        if (width < 1.0f)
        {
            width = 1.0f;
        }
        if (height < 1.0f)
        {
            height = 1.0f;
        }
        setBox(button, Rect(0.0f, 0.0f, floorf(width + 0.5f), floorf(height + 0.5f)));
        return button;
    }

    std::shared_ptr<UIButton> makeDangerButton(const String& label, const String& font, float size, float minWidth, float minHeight)
    {
        const RoseTheme& t = theme();
        auto button = makeButton(label, font, size, minWidth, minHeight, t.raised, t.danger);
        float unit = size / 15.0f;
        if (unit < 1.0f)
        {
            unit = 1.0f;
        }
        button->border.color = t.danger;
        button->border.thickness = unit;
        return button;
    }

    std::shared_ptr<UIButton> makeMoreButton(const String& font, float size, float width, float height, const Color& ink)
    {
        auto button = makeButton("...", font, size, width, height, Color(0.0f, 0.0f, 0.0f, 0.0f), ink);
        button->paddingX = 0.0f;
        button->paddingY = 0.0f;
        button->hoverFill = theme().overlayHover;
        button->pressedFill = theme().overlayPress;
        setBox(button, Rect(0.0f, 0.0f, width, height));
        return button;
    }

    std::shared_ptr<UITag> makeChip(const String& label, const String& font, float size, const Color& fill, const Color& ink)
    {
        auto chip = std::make_shared<UITag>();
        chip->shape.pivot = Vector2(0.0f, 0.0f);
        chip->paddingX = 8.0f;
        chip->paddingY = 3.0f;
        chip->background = fill;
        chip->caption().text = label;
        styleText(chip->caption(), font, size, ink);
        chip->layout();
        return chip;
    }

    std::shared_ptr<UIWidget> makeRule(float width, const Color& color)
    {
        auto rule = std::make_shared<UIWidget>();
        rule->pickable = false;
        rule->color = color;
        rule->shape.pivot = Vector2(0.0f, 0.0f);
        setBox(rule, Rect(0.0f, 0.0f, width, 1.0f));
        return rule;
    }

    void fillSurface(UIWidget& widget, const Color& color)
    {
        widget.color = color;
    }

    std::shared_ptr<UIWidget> makeSplit(float length, bool vertical)
    {
        auto split = std::make_shared<UIWidget>();
        split->pickable = false;
        split->color = theme().line;
        split->shape.pivot = Vector2(0.0f, 0.0f);
        if (vertical)
        {
            setBox(split, Rect(0.0f, 0.0f, 1.0f, length));
        }
        else
        {
            setBox(split, Rect(0.0f, 0.0f, length, 1.0f));
        }
        return split;
    }

    void styleDropdown(UIDropdown& drop, const String& font, float size)
    {
        const RoseTheme& t = theme();
        drop.background = t.field;
        drop.hoverFill = hoverOf(t.field);
        drop.pressedFill = pressOf(t.field);
        drop.border.color = t.ring;
        drop.border.thickness = 1.0f;
        drop.text.color = t.ink;
        drop.placeholderStyle.color = t.mute;
        drop.chevron = t.mute;
        drop.popup = t.popup;
        drop.itemHover = t.quiet;
        drop.itemPressed = pressOf(t.quiet);
        drop.itemSelected = t.accent;
        drop.itemInk = t.ink;
        drop.itemSelectedInk = t.inkOnAccent;
        if (UIText* caption = drop.caption())
        {
            styleText(*caption, font, size, t.ink);
        }
        if (drop.dropdown != nullptr)
        {
            drop.dropdown->color = t.popup;
        }
    }

    void stylePopupMenu(UIMenu& menu, float scale)
    {
        const RoseTheme& t = theme();
        menu.color = t.popup;
        menu.padding = 4.0f * scale;
        menu.spacing = 0.0f;
        menu.border.color = t.hair;
        menu.border.thickness = 1.0f;
    }

    void styleMenuItem(UIMenuItem& item, const String& font, float size, float scale)
    {
        const RoseTheme& t = theme();
        item.padding = 8.0f * scale;
        item.hoverInset = 4.0f * scale;
        item.background = Color(0.0f, 0.0f, 0.0f, 0.0f);
        item.hoverFill = t.quiet;
        item.selectedFill = t.selected;
        if (UIText* text = item.label())
        {
            styleText(*text, font, size, t.ink);
        }
    }

    void styleMenuBarItem(UIMenuItem& item, const String& font, float size, const Color& barBg)
    {
        const RoseTheme& t = theme();
        item.background = barBg;
        item.hoverFill = t.light ? t.quiet : hoverOf(barBg);
        item.selectedFill = t.selected;
        if (UIText* text = item.label())
        {
            styleText(*text, font, size, t.ink);
        }
    }

    PaintPanel::PaintPanel()
        : tl(0.0f, 0.0f, 0.0f, 0.0f)
        , tr(0.0f, 0.0f, 0.0f, 0.0f)
        , br(0.0f, 0.0f, 0.0f, 0.0f)
        , bl(0.0f, 0.0f, 0.0f, 0.0f)
        , accent(0.0f, 0.0f, 0.0f, 0.0f)
        , underline(0.0f, 0.0f, 0.0f, 0.0f)
        , hair(1.0f, 1.0f, 1.0f, 0.0f)
    {
        pickable = false;
        color = Color(0.0f, 0.0f, 0.0f, 0.0f);
        shape.pivot = Vector2(0.0f, 0.0f);
    }

    void PaintPanel::fill(const Color& color)
    {
        tl = tr = br = bl = color;
    }

    void PaintPanel::fill(const Color& top, const Color& bottom)
    {
        tl = tr = top;
        bl = br = bottom;
    }

    void PaintPanel::addDisc(UIPrimitive& primitive, float cx, float cy, float radius, const Color& color)
    {
        if (radius <= 1.0f || color.a <= 0.001f)
        {
            return;
        }
        constexpr int kSeg = 28;
        Rect uv = UIFont::solidUV();
        Vector2 center(cx, cy);
        for (int i = 0; i < kSeg; ++i)
        {
            float a0 = 6.28318530718f * ((float)i / (float)kSeg);
            float a1 = 6.28318530718f * ((float)(i + 1) / (float)kSeg);
            Vector2 e0(cx + cosf(a0) * radius, cy + sinf(a0) * radius);
            Vector2 e1(cx + cosf(a1) * radius, cy + sinf(a1) * radius);
            primitive.addQuad(
                this->localToScreen(center),
                this->localToScreen(e0),
                this->localToScreen(e1),
                this->localToScreen(center),
                uv,
                color);
        }
    }

    void PaintPanel::addGlow(UIPrimitive& primitive, const Orb& orb, float alphaScale)
    {
        Color c = orb.color;
        c.a *= alphaScale;
        if (c.a <= 0.001f)
        {
            return;
        }
        constexpr int kLayers = 7;
        for (int i = kLayers; i >= 1; --i)
        {
            float layerT = (float)i / (float)kLayers;
            Color layer = c;
            float falloff = (1.0f - layerT) * (1.0f - layerT);
            layer.a = c.a * (0.12f + 0.88f * falloff);
            if (layer.a > 0.01f)
            {
                this->addDisc(primitive, orb.x, orb.y, orb.r * layerT, layer);
            }
        }
    }

    void PaintPanel::render(UIPrimitive& primitive)
    {
        if (!visible)
        {
            return;
        }
        const bool hot = hoverLift && this->isHovered();
        const float lift = hot ? theme().cardMix : 0.0f;
        const bool darken = theme().light;
        auto tint = [lift, darken](const Color& color)
        {
            return darken ? mixBlack(color, lift) : liftColor(color, lift);
        };
        Color a = tint(tl);
        Color b = tint(tr);
        Color c = tint(br);
        Color d = tint(bl);
        primitive.addQuad(
            this->matrixLocalToScreen(),
            Rect(0.0f, 0.0f, shape.size.x, shape.size.y),
            UIFont::solidUV(),
            a, b, c, d);
        if (hair.a > 0.0f)
        {
            const float w = shape.size.x;
            const float h = shape.size.y;
            auto edge = [&](float x, float y, float ew, float eh)
            {
                primitive.addQuad(
                    this->matrixLocalToScreen(),
                    Rect(x, y, ew, eh),
                    UIFont::solidUV(),
                    hair);
            };
            if (framed)
            {
                edge(0.0f, 0.0f, w, 1.0f);
                edge(0.0f, h - 1.0f, w, 1.0f);
                edge(0.0f, 0.0f, 1.0f, h);
                edge(w - 1.0f, 0.0f, 1.0f, h);
            }
            else
            {
                edge(0.0f, 0.0f, w, 1.0f);
            }
        }
        if (accentW > 0.0f && accent.a > 0.0f)
        {
            primitive.addQuad(
                this->matrixLocalToScreen(),
                Rect(0.0f, 0.0f, accentW, shape.size.y),
                UIFont::solidUV(),
                accent);
        }
        if (underlineH > 0.0f && underline.a > 0.0f)
        {
            primitive.addQuad(
                this->matrixLocalToScreen(),
                Rect(0.0f, shape.size.y - underlineH, shape.size.x, underlineH),
                UIFont::solidUV(),
                underline);
        }
        if (!orbs.empty())
        {
            const float now = std::chrono::duration<float>(std::chrono::steady_clock::now().time_since_epoch()).count();
            for (const Orb& orb : orbs)
            {
                float scale = 1.0f;
                if (orb.pulse)
                {
                    scale = 0.78f + 0.22f * (0.5f + 0.5f * sinf(now * 1.35f + orb.phase));
                }
                this->addGlow(primitive, orb, scale);
            }
        }
        UIWidget::render(primitive);
    }

}
