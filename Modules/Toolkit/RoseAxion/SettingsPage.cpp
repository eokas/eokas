#include "./SettingsPage.h"
#include "./Theme.h"
#include "./UiKit.h"

#include "UI/Main.h"

namespace eokas::RoseAxion {

    void RoseSettingsPage::bind(const Host& host)
    {
        mHost = host;
    }

    void RoseSettingsPage::rebuild()
    {
        this->detachChildren();
        const RoseTheme& t = theme();
        fillSurface(*this, t.voidBg);
        this->shape.pivot = Vector2(0.0f, 0.0f);
        const float s = hostScale(mHost.scale);
        const float pad = 28.0f * s;
        auto title = makeText("Appearance", mHost.font, titleLgPx(s), t.ink, 400.0f * s, 28.0f * s);
        title->layout();
        setBox(title, Rect(pad, pad, title->shape.size.x, title->shape.size.y));
        this->attachChild(title);
        auto sub = makeText("Light, Black, and UE5 workspace themes. Takes effect immediately and your choice is remembered.", mHost.font, bodyPx(s), t.mute, 560.0f * s, 20.0f * s);
        sub->layout();
        setBox(sub, Rect(pad, pad + title->shape.size.y + 8.0f * s, sub->shape.size.x, sub->shape.size.y));
        this->attachChild(sub);

        const float cardW = 232.0f * s;
        const float cardH = 176.0f * s;
        const float gap = 16.0f * s;
        float y = pad + title->shape.size.y + 8.0f * s + sub->shape.size.y + 24.0f * s;
        for (int i = 0; i < roseThemeCount(); ++i)
        {
            const RoseThemeId id = roseThemeAt(i);
            const RoseTheme& preview = themeById(id);
            const bool on = themeId() == id;
            auto card = std::make_shared<PaintPanel>();
            card->pickable = true;
            card->hoverLift = true;
            card->fill(preview.canvas);
            card->hair = preview.hair;
            card->framed = true;
            card->accent = preview.accentBright;
            card->accentW = on ? 4.0f * s : 0.0f;
            setBox(card, Rect(pad + (cardW + gap) * (float)i, y, cardW, cardH));

            auto bar = std::make_shared<UIWidget>();
            bar->pickable = false;
            bar->color = preview.bar;
            setBox(bar, Rect(0.0f, 0.0f, cardW, 14.0f * s));
            card->attachChild(bar);

            auto hero = std::make_shared<PaintPanel>();
            hero->fill(preview.paper);
            hero->hair = preview.hair;
            setBox(hero, Rect(12.0f * s, 22.0f * s, cardW - 24.0f * s, 28.0f * s));
            card->attachChild(hero);

            const float sw = 18.0f * s;
            const float swY = 58.0f * s;
            auto swatchA = std::make_shared<UIWidget>();
            swatchA->pickable = false;
            swatchA->color = preview.accent;
            setBox(swatchA, Rect(16.0f * s, swY, sw, sw));
            card->attachChild(swatchA);
            auto swatchB = std::make_shared<UIWidget>();
            swatchB->pickable = false;
            swatchB->color = preview.secondary;
            setBox(swatchB, Rect(16.0f * s + sw + 6.0f * s, swY, sw, sw));
            card->attachChild(swatchB);
            auto swatchC = std::make_shared<UIWidget>();
            swatchC->pickable = false;
            swatchC->color = preview.paper;
            setBox(swatchC, Rect(16.0f * s + (sw + 6.0f * s) * 2.0f, swY, sw, sw));
            card->attachChild(swatchC);

            auto name = makeText(preview.name, mHost.font, titleMdPx(s), preview.ink, cardW - 32.0f * s, 22.0f * s);
            name->layout();
            setBox(name, Rect(16.0f * s, 84.0f * s, name->shape.size.x, name->shape.size.y));
            card->attachChild(name);
            auto blurb = makeText(preview.blurb, mHost.font, bodyPx(s), preview.mute, cardW - 32.0f * s, 40.0f * s);
            blurb->layout();
            setBox(blurb, Rect(16.0f * s, 110.0f * s, blurb->shape.size.x, blurb->shape.size.y));
            card->attachChild(blurb);
            card->onClick = [this, id]()
            {
                if (setRoseThemeId(id) && mHost.requestRebuild)
                {
                    mHost.requestRebuild();
                }
            };
            this->attachChild(card);
        }
    }

    void RoseSettingsPage::layout()
    {
        if (!visible)
        {
            return;
        }
        UIWidget::layout();
    }

}
