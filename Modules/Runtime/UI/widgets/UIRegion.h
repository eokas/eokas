#ifndef _EOKAS_UI_REGION_H_
#define _EOKAS_UI_REGION_H_

#include "../UIWidget.h"

namespace eokas
{
    class UIRegion : public UIWidget
    {
    public:
        UIRegion();
        bool expanded() const { return mExpanded; }
        void setExpanded(bool next);
        float spacing() const { return mSpacing; }
        void setSpacing(float value);
        const UIWidget::Ref& head() const { return mHead; }
        void setHead(const UIWidget::Ref& widget);
        const UIWidget::Ref& body() const { return mBody; }
        void setBody(const UIWidget::Ref& widget);
        void layout() override;
        void render(UIPrimitive& primitive) override;

        std::function<void(bool)> onExpandedChanged;

    private:
        bool mExpanded = true;
        float mSpacing = 0.0f;
        UIWidget::Ref mHead;
        UIWidget::Ref mBody;

        void syncChildren();
    };
}

#endif//_EOKAS_UI_REGION_H_
