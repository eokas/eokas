#include "./SearchPage.h"
#include "./Theme.h"
#include "./UiKit.h"

#include "UI/Main.h"

namespace eokas::RoseAxion {

    void RoseSearchPage::bind(const Host& host)
    {
        mHost = host;
    }

    void RoseSearchPage::rebuild()
    {
        this->detachChildren();
        const RoseTheme& t = theme();
        fillSurface(*this, t.voidBg);
        this->shape.pivot = Vector2(0.0f, 0.0f);
        const float s = hostScale(mHost.scale);
        auto hint = makeText("Search coming soon", mHost.font, bodyPx(s), t.mute, 280.0f * s, 24.0f * s);
        hint->layout();
        setBox(hint, Rect(24.0f * s, 24.0f * s, hint->shape.size.x, hint->shape.size.y));
        this->attachChild(hint);
    }

    void RoseSearchPage::layout()
    {
        if (!visible)
        {
            return;
        }
        UIWidget::layout();
    }

}
