#ifndef _EOKAS_ROSE_AXION_UIKIT_H_
#define _EOKAS_ROSE_AXION_UIKIT_H_

#include "./Theme.h"
#include "UI/Main.h"

#include <vector>

namespace eokas::RoseAxion {

    void setBox(UIWidget& widget, const Rect& box);
    void setBox(const std::shared_ptr<UIWidget>& widget, const Rect& box);
    void placeOnOverlay(UIWidget& overlay, UIWidget& widget, const Rect& screen, bool below = false);
    void placePopup(UIWidget& overlay, UIWidget& menu, const Rect& anchor);
    String fileTitleFromPath(const String& path);
    void styleText(UIText& text, const String& font, float size, const Color& color);

    std::shared_ptr<UIText> makeText(const String& value, const String& font, float size, const Color& color, float width, float height);
    std::shared_ptr<UIButton> makeButton(const String& label, const String& font, float size, float minWidth, float minHeight, const Color& fill, const Color& ink);
    std::shared_ptr<UIButton> makeDangerButton(const String& label, const String& font, float size, float minWidth, float minHeight);
    std::shared_ptr<UIButton> makeMoreButton(const String& font, float size, float width, float height, const Color& ink);
    std::shared_ptr<UITag> makeChip(const String& label, const String& font, float size, const Color& fill, const Color& ink);
    std::shared_ptr<UIWidget> makeRule(float width, const Color& color);
    void fillSurface(UIWidget& widget, const Color& color);
    std::shared_ptr<UIWidget> makeSplit(float length, bool vertical);
    void styleDropdown(UIDropdown& drop, const String& font, float size);
    void styleMenuItem(UIMenuItem& item, const String& font, float size);
    void styleMenuBarItem(UIMenuItem& item, const String& font, float size, const Color& barBg);

    inline float menuFontPx(float scale)
    {
        return 13.0f * scale;
    }

    class PaintPanel : public UIWidget
    {
    public:
        Color tl;
        Color tr;
        Color br;
        Color bl;
        Color accent;
        float accentW = 0.0f;
        Color underline;
        float underlineH = 0.0f;
        Color hair;
        bool framed = false;
        bool hoverLift = false;

        struct Orb
        {
            float x = 0.0f;
            float y = 0.0f;
            float r = 0.0f;
            Color color;
            float phase = 0.0f;
            bool pulse = false;
        };
        std::vector<Orb> orbs;

        PaintPanel();
        void fill(const Color& color);
        void fill(const Color& top, const Color& bottom);
        void render(UIPrimitive& primitive) override;

    private:
        void addDisc(UIPrimitive& primitive, float cx, float cy, float radius, const Color& color);
        void addGlow(UIPrimitive& primitive, const Orb& orb, float alphaScale);
    };

}

#endif
