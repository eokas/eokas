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
        const std::shared_ptr<UIWidget>& head() const { return mHead; }
        void setHead(const std::shared_ptr<UIWidget>& widget);
        const std::shared_ptr<UIWidget>& body() const { return mBody; }
        void setBody(const std::shared_ptr<UIWidget>& widget);
        void layout(const Rect& rect) override;
        void render(UIPrimitive& primitive) override;

        std::function<void(bool)> onExpandedChanged;

    private:
        bool mExpanded = true;
        float mSpacing = 0.0f;
        std::shared_ptr<UIWidget> mHead;
        std::shared_ptr<UIWidget> mBody;

        void syncChildren();
    };
}

#endif//_EOKAS_UI_REGION_H_
