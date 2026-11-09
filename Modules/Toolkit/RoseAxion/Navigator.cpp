#include "./Navigator.h"
#include "./Theme.h"
#include "./UiKit.h"

#include "UI/Main.h"

#include <cmath>

namespace eokas::RoseAxion {

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
        return mHost.scale < 0.5f ? 1.0f : mHost.scale;
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
        const float pad = 6.0f * s;
        const float width = shape.size.x;
        const float height = shape.size.y;
        const float btnW = width - pad * 2.0f;
        const float btnH = 40.0f * s;
        const float gap = 4.0f * s;
        const float stride = btnH + gap;
        if (mHome != nullptr)
        {
            setBox(*mHome, Rect(pad, pad, btnW, btnH));
        }
        if (mTables != nullptr)
        {
            setBox(*mTables, Rect(pad, pad + stride, btnW, btnH));
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
        mTables = nullptr;
        mSchemas = nullptr;
        mSearch = nullptr;
        mSettings = nullptr;

        const float s = this->scale();
        const float fontPx = 11.0f * s;
        const RoseTheme& t = theme();
        const Color bar = t.bar;
        const Color selected = t.navActive;
        const Color quiet = t.navIdle;
        const Color ink = t.ink;
        const Color line = t.line;
        this->color = bar;
        this->shape.pivot = Vector2(0.0f, 0.0f);

        auto add = [this, fontPx, selected, quiet, ink, s, &t](const char* label, EditorPage page, UIWidget*& slot)
        {
            const bool on = mHost.current == page;
            const float luma = 0.2126f * selected.r + 0.7152f * selected.g + 0.0722f * selected.b;
            const Color onInk = (t.light || luma < 0.45f) ? ink : t.inkOnAccent;
            auto button = makeButton(label, this->font(), fontPx, 44.0f * this->scale(), 40.0f * this->scale(), on ? selected : quiet, on ? onInk : ink);
            button->paddingX = 4.0f;
            button->paddingY = 6.0f;
            if (UIText* text = button->label())
            {
                text->maxWidth = 44.0f * this->scale() - 8.0f;
            }
            if (on)
            {
                auto mark = std::make_shared<UIWidget>();
                mark->pickable = false;
                mark->color = t.light ? t.accent : t.accentBright;
                setBox(mark, Rect(0.0f, 0.0f, 3.0f * s, 40.0f * s));
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
        add("Home", EditorPage::Home, mHome);
        add("Tables", EditorPage::Tables, mTables);
        add("Schemas", EditorPage::Schemas, mSchemas);
        add("Search", EditorPage::Search, mSearch);
        add("Settings", EditorPage::Settings, mSettings);

        auto rule = std::make_shared<UIWidget>();
        rule->pickable = false;
        rule->color = line;
        rule->shape.pivot = Vector2(0.0f, 0.0f);
        setBox(rule, Rect(shape.size.x - 1.0f, 0.0f, 1.0f, shape.size.y));
        this->attachChild(rule);
        this->layout();
    }

}
