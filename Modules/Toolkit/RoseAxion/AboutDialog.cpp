#include "./AboutDialog.h"
#include "./Header.h"
#include "./Theme.h"
#include "./UiKit.h"

#include "Rose/Library.h"
#include "UI/Main.h"
#include "UI/UIFont.h"

#include <cmath>

namespace eokas::RoseAxion {

    namespace {

        class AboutMark : public UIWidget
        {
        public:
            Color fill;
            Color rim;
            float rimWidth = 2.0f;

            AboutMark()
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
                for (auto& child : children())
                {
                    if (!child || !child->visible)
                    {
                        continue;
                    }
                    child->layout();
                    float x = (shape.size.x - child->shape.size.x) * 0.5f;
                    float y = (shape.size.y - child->shape.size.y) * 0.5f;
                    this->placeChild(*child, Vector2(floorf(x + 0.5f), floorf(y + 0.5f)));
                }
            }

            void render(UIPrimitive& primitive) override
            {
                if (!visible)
                {
                    return;
                }
                float radius = shape.size.x < shape.size.y ? shape.size.x : shape.size.y;
                radius *= 0.5f;
                float cx = shape.size.x * 0.5f;
                float cy = shape.size.y * 0.5f;
                if (rim.a > 0.001f && rimWidth > 0.0f)
                {
                    this->addDisc(primitive, cx, cy, radius, rim);
                    float inner = radius - rimWidth;
                    if (inner > 1.0f)
                    {
                        this->addDisc(primitive, cx, cy, inner, fill);
                    }
                    else
                    {
                        this->addDisc(primitive, cx, cy, radius, fill);
                    }
                }
                else
                {
                    this->addDisc(primitive, cx, cy, radius, fill);
                }
                UIWidget::render(primitive);
            }

        private:
            void addDisc(UIPrimitive& primitive, float cx, float cy, float radius, const Color& color)
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
        };

        String repoLabel()
        {
            String url = kRepoUrl;
            if (url.startsWith("https://"))
            {
                return url.substr(8);
            }
            if (url.startsWith("http://"))
            {
                return url.substr(7);
            }
            return url;
        }

        void placeInRow(UIWidget& row, UIWidget& label, UIWidget& value, float width)
        {
            float rowH = label.shape.size.y;
            if (value.shape.size.y > rowH)
            {
                rowH = value.shape.size.y;
            }
            setBox(row, Rect(0.0f, 0.0f, width, rowH));
            setBox(label, Rect(0.0f, (rowH - label.shape.size.y) * 0.5f, label.shape.size.x, label.shape.size.y));
            float valueX = width - value.shape.size.x;
            if (valueX < label.shape.size.x + 12.0f)
            {
                valueX = label.shape.size.x + 12.0f;
            }
            setBox(value, Rect(valueX, (rowH - value.shape.size.y) * 0.5f, value.shape.size.x, value.shape.size.y));
        }

    }

    void RoseAboutDialog::bind(const Host& host)
    {
        mHost = host;
    }

    void RoseAboutDialog::close()
    {
        if (mHost.onClose)
        {
            mHost.onClose();
        }
    }

    bool RoseAboutDialog::handleEscape()
    {
        this->close();
        return true;
    }

    UIWidget* RoseAboutDialog::focusWidget() const
    {
        return mFocus;
    }

    void RoseAboutDialog::rebuild(float overlayW, float overlayH)
    {
        this->detachChildren();
        mFocus = nullptr;

        const RoseTheme& t = theme();
        const float s = hostScale(mHost.scale);
        const float fontPx = 15.0f * s;
        const float smallPx = 13.0f * s;
        const float titlePx = 20.0f * s;
        const float iconPx = 52.0f * s;
        const float padX = 28.0f * s;
        const float headPadY = 22.0f * s;
        const float bodyPadY = 16.0f * s;
        const float footPadY = 14.0f * s;
        const float identityGap = 16.0f * s;
        const float rowGap = 10.0f * s;
        const Color ink = t.ink;
        const Color mute = t.mute;
        const Color clear(0.0f, 0.0f, 0.0f, 0.0f);

        this->overlay = true;
        this->pickable = true;
        this->color = t.shade;
        this->shape.pivot = Vector2(0.0f, 0.0f);
        setBox(*this, Rect(0.0f, 0.0f, overlayW, overlayH));
        this->onClick = [this]() { this->close(); };

        float panelW = 400.0f * s;
        float maxW = overlayW - 32.0f * s;
        if (maxW < 240.0f * s)
        {
            maxW = overlayW;
        }
        if (panelW > maxW)
        {
            panelW = maxW;
        }
        float innerW = panelW - padX * 2.0f;
        if (innerW < 160.0f * s)
        {
            innerW = 160.0f * s;
            panelW = innerW + padX * 2.0f;
        }

        auto mark = std::make_shared<AboutMark>();
        mark->fill = t.accent;
        mark->rim = mixWhite(t.accent, t.light ? 0.28f : 0.18f);
        mark->rimWidth = 2.0f * s;
        setBox(mark, Rect(0.0f, 0.0f, iconPx, iconPx));
        auto glyph = makeText("R", mHost.font, 20.0f * s, t.inkOnAccent, iconPx, 0.0f);
        glyph->layout();
        mark->attachChild(glyph);
        mark->layout();

        auto name = makeText(kAppName, mHost.font, titlePx, ink, innerW - iconPx - identityGap, 0.0f);
        name->layout();
        auto tagline = makeText("Rose Library 编辑器", mHost.font, smallPx, mute, innerW - iconPx - identityGap, 0.0f);
        tagline->layout();

        float textBlockH = name->shape.size.y + 6.0f * s + tagline->shape.size.y;
        float heroH = iconPx;
        if (textBlockH > heroH)
        {
            heroH = textBlockH;
        }
        float headerH = headPadY + heroH + headPadY;

        auto versionLabel = makeText("版本", mHost.font, smallPx, mute, innerW * 0.45f, 0.0f);
        versionLabel->layout();
        auto versionValue = makeText(kAppVersion, mHost.font, fontPx, ink, innerW * 0.55f, 0.0f);
        versionValue->layout();
        auto roseLabel = makeText("Rose 版本", mHost.font, smallPx, mute, innerW * 0.45f, 0.0f);
        roseLabel->layout();
        auto roseValue = makeText(String::format("%d", rose::Library::version), mHost.font, fontPx, ink, innerW * 0.55f, 0.0f);
        roseValue->layout();
        auto repoName = makeText("项目主页", mHost.font, smallPx, mute, innerW * 0.40f, 0.0f);
        repoName->layout();
        auto repoValue = makeText(repoLabel(), mHost.font, fontPx, t.url, innerW * 0.60f, 0.0f);
        repoValue->pickable = true;
        repoValue->onClick = [this]()
        {
            if (mHost.openExternal)
            {
                mHost.openExternal(kRepoUrl);
            }
        };
        UIText* repoRaw = repoValue.get();
        Color urlIdle = t.url;
        Color urlHot = t.accentBright;
        repoValue->onPointerEnter = [repoRaw, urlHot](const UIPointerEvent&)
        {
            if (repoRaw != nullptr)
            {
                repoRaw->style.color = urlHot;
            }
        };
        repoValue->onPointerLeave = [repoRaw, urlIdle](const UIPointerEvent&)
        {
            if (repoRaw != nullptr)
            {
                repoRaw->style.color = urlIdle;
            }
        };
        repoValue->layout();

        auto versionRow = std::make_shared<UIWidget>();
        versionRow->pickable = false;
        versionRow->color = clear;
        versionRow->shape.pivot = Vector2(0.0f, 0.0f);
        placeInRow(*versionRow, *versionLabel, *versionValue, innerW);
        versionRow->attachChild(versionLabel);
        versionRow->attachChild(versionValue);

        auto roseRow = std::make_shared<UIWidget>();
        roseRow->pickable = false;
        roseRow->color = clear;
        roseRow->shape.pivot = Vector2(0.0f, 0.0f);
        placeInRow(*roseRow, *roseLabel, *roseValue, innerW);
        roseRow->attachChild(roseLabel);
        roseRow->attachChild(roseValue);

        auto repoRow = std::make_shared<UIWidget>();
        repoRow->pickable = false;
        repoRow->color = clear;
        repoRow->shape.pivot = Vector2(0.0f, 0.0f);
        placeInRow(*repoRow, *repoName, *repoValue, innerW);
        repoRow->attachChild(repoName);
        repoRow->attachChild(repoValue);

        float bodyH = bodyPadY + versionRow->shape.size.y + rowGap + roseRow->shape.size.y + rowGap
            + repoRow->shape.size.y + bodyPadY;

        auto copy = makeText("Copyright \xC2\xA9 2026 eokas", mHost.font, smallPx, mute, innerW, 0.0f);
        copy->layout();
        auto ok = makeButton("确定", mHost.font, fontPx, 88.0f * s, 32.0f * s, t.accent, t.inkOnAccent);
        ok->onClick = [this]() { this->close(); };
        ok->onKeyPress = [this](const UIKeyEvent& event)
        {
            if (event.key == UIKeyCode::Escape || event.key == UIKeyCode::Enter)
            {
                this->handleEscape();
            }
        };

        float footInnerH = copy->shape.size.y;
        if (ok->shape.size.y > footInnerH)
        {
            footInnerH = ok->shape.size.y;
        }
        float footerH = footPadY + footInnerH + footPadY;
        float panelH = headerH + bodyH + footerH;

        float panelX = (overlayW - panelW) * 0.5f;
        float panelY = (overlayH - panelH) * 0.5f;
        if (panelX < 16.0f * s)
        {
            panelX = 16.0f * s;
        }
        if (panelY < 16.0f * s)
        {
            panelY = 16.0f * s;
        }

        auto panel = std::make_shared<PaintPanel>();
        panel->pickable = true;
        panel->fill(t.raised);
        panel->hair = t.hair;
        panel->framed = true;
        panel->shape.pivot = Vector2(0.0f, 0.0f);
        setBox(panel, Rect(panelX, panelY, panelW, panelH));

        auto header = std::make_shared<PaintPanel>();
        header->pickable = false;
        header->fill(t.header);
        header->underline = t.accent;
        header->underlineH = 2.0f * s;
        header->shape.pivot = Vector2(0.0f, 0.0f);
        setBox(header, Rect(0.0f, 0.0f, panelW, headerH));

        float markY = headPadY + (heroH - iconPx) * 0.5f;
        float textY = headPadY + (heroH - textBlockH) * 0.5f;
        float textX = padX + iconPx + identityGap;
        setBox(mark, Rect(padX, markY, iconPx, iconPx));
        setBox(name, Rect(textX, textY, name->shape.size.x, name->shape.size.y));
        setBox(tagline, Rect(textX, textY + name->shape.size.y + 6.0f * s, tagline->shape.size.x, tagline->shape.size.y));
        header->attachChild(mark);
        header->attachChild(name);
        header->attachChild(tagline);

        auto body = std::make_shared<UIWidget>();
        body->pickable = false;
        body->color = clear;
        body->shape.pivot = Vector2(0.0f, 0.0f);
        setBox(body, Rect(0.0f, headerH, panelW, bodyH));
        float rowY = bodyPadY;
        setBox(versionRow, Rect(padX, rowY, innerW, versionRow->shape.size.y));
        rowY += versionRow->shape.size.y + rowGap;
        setBox(roseRow, Rect(padX, rowY, innerW, roseRow->shape.size.y));
        rowY += roseRow->shape.size.y + rowGap;
        setBox(repoRow, Rect(padX, rowY, innerW, repoRow->shape.size.y));
        body->attachChild(versionRow);
        body->attachChild(roseRow);
        body->attachChild(repoRow);

        auto footer = std::make_shared<UIWidget>();
        footer->pickable = false;
        footer->color = t.header;
        footer->shape.pivot = Vector2(0.0f, 0.0f);
        setBox(footer, Rect(0.0f, headerH + bodyH, panelW, footerH));
        auto footRule = makeRule(panelW, t.line);
        setBox(footRule, Rect(0.0f, 0.0f, panelW, 1.0f));
        float copyY = footPadY + (footInnerH - copy->shape.size.y) * 0.5f;
        float okY = footPadY + (footInnerH - ok->shape.size.y) * 0.5f;
        setBox(copy, Rect(padX, copyY, copy->shape.size.x, copy->shape.size.y));
        setBox(ok, Rect(panelW - padX - ok->shape.size.x, okY, ok->shape.size.x, ok->shape.size.y));
        footer->attachChild(footRule);
        footer->attachChild(copy);
        footer->attachChild(ok);

        panel->attachChild(header);
        panel->attachChild(body);
        panel->attachChild(footer);
        this->attachChild(panel);
        mFocus = ok.get();
    }

}
