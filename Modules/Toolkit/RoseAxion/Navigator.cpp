#include "./Navigator.h"
#include "./Theme.h"
#include "./UiKit.h"

#include "UI/Main.h"
#include "UI/UIStroke.h"

#include <cmath>
#include <vector>

namespace eokas::RoseAxion {

    namespace {

        UIStrokeStyle navStroke(const Color& ink, float size)
        {
            UIStrokeStyle stroke;
            stroke.color = ink;
            stroke.thickness = size * (1.5f / 18.0f);
            if (stroke.thickness < 1.0f)
            {
                stroke.thickness = 1.0f;
            }
            return stroke;
        }

        Vector2 navPt(float x, float y, float unit)
        {
            return Vector2(x * unit, y * unit);
        }

        void drawHome(UIPrimitive& primitive, const Matrix3& localToScreen, float size, const Color& ink)
        {
            const UIStrokeStyle stroke = navStroke(ink, size);
            const float u = size / 18.0f;
            const std::vector<Vector2> roof = { navPt(3.0f, 9.5f, u), navPt(9.0f, 3.2f, u), navPt(15.0f, 9.5f, u) };
            UIStroke::path(primitive, localToScreen, roof, false, stroke);
            UIStroke::border(primitive, localToScreen, Rect(4.5f * u, 9.0f * u, 9.0f * u, 6.5f * u), stroke);
            UIStroke::border(primitive, localToScreen, Rect(8.0f * u, 11.5f * u, 2.0f * u, 4.0f * u), stroke);
        }

        void drawInstance(UIPrimitive& primitive, const Matrix3& localToScreen, float size, const Color& ink)
        {
            const UIStrokeStyle stroke = navStroke(ink, size);
            const float u = size / 18.0f;
            UIStroke::border(primitive, localToScreen, Rect(3.5f * u, 3.5f * u, 11.0f * u, 11.0f * u), stroke);
            constexpr int kSeg = 16;
            const float cx = 12.5f * u;
            const float cy = 12.5f * u;
            const float r = 2.2f * u;
            std::vector<Vector2> ring;
            ring.reserve((size_t)kSeg);
            for (int i = 0; i < kSeg; ++i)
            {
                float a = 6.28318530718f * ((float)i / (float)kSeg);
                ring.push_back(Vector2(cx + cosf(a) * r, cy + sinf(a) * r));
            }
            UIStroke::path(primitive, localToScreen, ring, true, stroke);
        }

        void drawSchemas(UIPrimitive& primitive, const Matrix3& localToScreen, float size, const Color& ink)
        {
            const UIStrokeStyle stroke = navStroke(ink, size);
            const float u = size / 18.0f;
            UIStroke::border(primitive, localToScreen, Rect(2.5f * u, 2.5f * u, 11.0f * u, 4.5f * u), stroke);
            UIStroke::border(primitive, localToScreen, Rect(4.0f * u, 6.75f * u, 11.0f * u, 4.5f * u), stroke);
            UIStroke::border(primitive, localToScreen, Rect(5.5f * u, 11.0f * u, 11.0f * u, 4.5f * u), stroke);
        }

        void drawSearch(UIPrimitive& primitive, const Matrix3& localToScreen, float size, const Color& ink)
        {
            const UIStrokeStyle stroke = navStroke(ink, size);
            const float u = size / 18.0f;
            const float cx = 7.5f * u;
            const float cy = 7.5f * u;
            const float radius = 4.2f * u;
            constexpr int kSeg = 24;
            std::vector<Vector2> ring;
            ring.reserve((size_t)kSeg);
            for (int i = 0; i < kSeg; ++i)
            {
                float a = 6.28318530718f * ((float)i / (float)kSeg);
                ring.push_back(Vector2(cx + cosf(a) * radius, cy + sinf(a) * radius));
            }
            UIStroke::path(primitive, localToScreen, ring, true, stroke);
            UIStroke::segment(primitive, localToScreen, navPt(10.6f, 10.6f, u), navPt(15.0f, 15.0f, u), stroke);
        }

        void drawSettings(UIPrimitive& primitive, const Matrix3& localToScreen, float size, const Color& ink)
        {
            const UIStrokeStyle stroke = navStroke(ink, size);
            const float u = size / 18.0f;
            UIStroke::segment(primitive, localToScreen, navPt(3.0f, 5.0f, u), navPt(15.0f, 5.0f, u), stroke);
            UIStroke::segment(primitive, localToScreen, navPt(3.0f, 9.0f, u), navPt(15.0f, 9.0f, u), stroke);
            UIStroke::segment(primitive, localToScreen, navPt(3.0f, 13.0f, u), navPt(15.0f, 13.0f, u), stroke);
            const float k = 2.2f * u;
            UIStroke::border(primitive, localToScreen, Rect(11.0f * u - k * 0.5f, 5.0f * u - k * 0.5f, k, k), stroke);
            UIStroke::border(primitive, localToScreen, Rect(7.0f * u - k * 0.5f, 9.0f * u - k * 0.5f, k, k), stroke);
            UIStroke::border(primitive, localToScreen, Rect(12.0f * u - k * 0.5f, 13.0f * u - k * 0.5f, k, k), stroke);
        }

        struct NavIcon : UIWidget
        {
            using Draw = void (*)(UIPrimitive&, const Matrix3&, float, const Color&);

            Draw draw = nullptr;
            Color ink;

            NavIcon()
            {
                pickable = false;
                color.a = 0.0f;
                shape.pivot = Vector2(0.0f, 0.0f);
            }

            void render(UIPrimitive& primitive) override
            {
                if (!visible || draw == nullptr)
                {
                    return;
                }
                float size = shape.size.x < shape.size.y ? shape.size.x : shape.size.y;
                draw(primitive, this->matrixLocalToScreen(), size, ink);
                UIWidget::render(primitive);
            }
        };

        struct NavStack : UIWidget
        {
            float gap = 2.0f;

            NavStack()
            {
                pickable = false;
                color.a = 0.0f;
                shape.pivot = Vector2(0.0f, 0.0f);
            }

            void layout() override
            {
                if (!visible)
                {
                    return;
                }
                UIWidget* iconWidget = nullptr;
                UIWidget* labelWidget = nullptr;
                for (auto& child : children())
                {
                    if (!child || !child->visible)
                    {
                        continue;
                    }
                    if (iconWidget == nullptr)
                    {
                        iconWidget = child.get();
                    }
                    else
                    {
                        labelWidget = child.get();
                        break;
                    }
                }
                if (iconWidget == nullptr || labelWidget == nullptr)
                {
                    return;
                }
                iconWidget->layout();
                labelWidget->layout();
                float width = iconWidget->shape.size.x;
                if (labelWidget->shape.size.x > width)
                {
                    width = labelWidget->shape.size.x;
                }
                float height = iconWidget->shape.size.y + gap + labelWidget->shape.size.y;
                this->resize(Vector2(floorf(width + 0.5f), floorf(height + 0.5f)));
                float iconX = (width - iconWidget->shape.size.x) * 0.5f;
                if (iconX < 0.0f)
                {
                    iconX = 0.0f;
                }
                float labelX = (width - labelWidget->shape.size.x) * 0.5f;
                if (labelX < 0.0f)
                {
                    labelX = 0.0f;
                }
                this->placeChild(*iconWidget, Vector2(floorf(iconX + 0.5f), 0.0f));
                this->placeChild(*labelWidget, Vector2(floorf(labelX + 0.5f), floorf(iconWidget->shape.size.y + gap + 0.5f)));
            }
        };

    }

    void RoseNavigator::bind(const Host& host)
    {
        mHost = host;
    }

    const String& RoseNavigator::font() const
    {
        return mHost.font;
    }

    float RoseNavigator::scale() const
    {
        return hostScale(mHost.scale);
    }

    float RoseNavigator::barWidth() const
    {
        return floorf(56.0f * this->scale() + 0.5f);
    }

    void RoseNavigator::layout()
    {
        if (!visible)
        {
            return;
        }
        const float s = this->scale();
        const float pad = kRoseNavSidePad * s;
        const float width = shape.size.x;
        const float height = shape.size.y;
        const float btnW = width - pad * 2.0f;
        const float btnH = 56.0f * s;
        const float gap = 4.0f * s;
        const float stride = btnH + gap;
        if (mHome != nullptr)
        {
            setBox(*mHome, Rect(pad, pad, btnW, btnH));
        }
        if (mInstance != nullptr)
        {
            setBox(*mInstance, Rect(pad, pad + stride, btnW, btnH));
        }
        if (mSchemas != nullptr)
        {
            setBox(*mSchemas, Rect(pad, pad + stride * 2.0f, btnW, btnH));
        }
        if (mSearch != nullptr)
        {
            setBox(*mSearch, Rect(pad, pad + stride * 3.0f, btnW, btnH));
        }
        if (mSettings != nullptr)
        {
            float y = height - pad - btnH;
            const float minY = pad + stride * 4.0f;
            if (y < minY)
            {
                y = minY;
            }
            setBox(*mSettings, Rect(pad, y, btnW, btnH));
        }
        UIWidget::layout();
    }

    void RoseNavigator::rebuild()
    {
        this->detachChildren();
        mHome = nullptr;
        mInstance = nullptr;
        mSchemas = nullptr;
        mSearch = nullptr;
        mSettings = nullptr;

        const float s = this->scale();
        const float labelPx = 9.0f * s;
        const float iconPx = 18.0f * s;
        const float btnH = 56.0f * s;
        const RoseTheme& t = theme();
        const Color bar = t.bar;
        const Color selected = t.navActive;
        const Color quiet = t.navIdle;
        const Color ink = t.ink;
        fillSurface(*this, bar);
        this->shape.pivot = Vector2(0.0f, 0.0f);

        using Draw = NavIcon::Draw;
        auto add = [this, labelPx, iconPx, btnH, selected, quiet, ink, s, &t](const char* label, Draw draw, EditorPage page, UIWidget*& slot)
        {
            const bool on = mHost.current == page;
            const float luma = 0.2126f * selected.r + 0.7152f * selected.g + 0.0722f * selected.b;
            const Color onInk = (t.light || luma < 0.45f) ? ink : t.inkOnAccent;
            const Color fill = on ? selected : quiet;
            const Color glyph = on ? onInk : ink;

            auto button = std::make_shared<UIButton>();
            button->shape.pivot = Vector2(0.0f, 0.0f);
            button->paddingX = 4.0f * s;
            button->paddingY = 6.0f * s;
            button->background = fill;
            button->hoverFill = hoverOf(fill);
            button->pressedFill = pressOf(fill);

            auto stack = std::make_shared<NavStack>();
            stack->gap = 2.0f * s;

            auto icon = std::make_shared<NavIcon>();
            icon->draw = draw;
            icon->ink = glyph;
            setBox(icon, Rect(0.0f, 0.0f, iconPx, iconPx));

            auto text = std::make_shared<UIText>();
            text->text = label;
            styleText(*text, this->font(), labelPx, glyph);
            text->maxWidth = 0.0f;

            stack->attachChild(icon);
            stack->attachChild(text);
            button->setContent(stack);

            if (on)
            {
                const float navPad = kRoseNavSidePad * s;
                auto mark = std::make_shared<UIWidget>();
                mark->pickable = false;
                mark->color = t.light ? t.accent : t.accentBright;
                setBox(mark, Rect(-navPad, 0.0f, kRoseNavMarkW * s, btnH));
                button->attachChild(mark);
            }
            button->onClick = [this, page]()
            {
                if (mHost.selectPage)
                {
                    mHost.selectPage(page);
                }
            };
            this->attachChild(button);
            slot = button.get();
        };
        add("Home", drawHome, EditorPage::Home, mHome);
        add("Instance", drawInstance, EditorPage::Instance, mInstance);
        add("Schemas", drawSchemas, EditorPage::Schemas, mSchemas);
        add("Search", drawSearch, EditorPage::Search, mSearch);
        add("Settings", drawSettings, EditorPage::Settings, mSettings);

        auto rule = makeSplit(shape.size.y, true);
        setBox(rule, Rect(shape.size.x - 1.0f, 0.0f, 1.0f, shape.size.y));
        this->attachChild(rule);
        this->layout();
    }

}
