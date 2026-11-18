#ifndef _EOKAS_ROSE_AXION_THEME_H_
#define _EOKAS_ROSE_AXION_THEME_H_

#include "Base/Color.h"
#include "Base/String.h"

namespace eokas::RoseAxion {

    enum class RoseThemeId
    {
        Light = 0,
        Black = 1,
        Unreal = 2,
    };

    struct RoseTheme
    {
        RoseThemeId id = RoseThemeId::Light;
        const char* key = "light";
        const char* name = "Light";
        const char* blurb = "";
        bool light = false;

        // background-color
        Color voidBg;
        Color canvas;
        Color paper;
        Color bar;
        Color header;
        Color raised;
        Color popup;
        Color field;
        Color quiet;
        Color navIdle;
        Color navActive;
        Color selected;
        Color selectedCell;
        Color selection;
        Color shade;

        // color
        Color ink;
        Color mute;
        Color inkOnAccent;
        Color url;

        // border-color / outline
        Color line;
        Color hair;
        Color gridLine;
        Color ring;

        // fills
        Color accent;
        Color accentBright;
        Color secondary;
        Color warn;
        Color danger;
        Color chip;

        // background-image layers
        Color heroTl;
        Color heroTr;
        Color heroBr;
        Color heroBl;
        Color cardTop;
        Color cardBottom;
        Color sectionTop;
        Color sectionBottom;
        Color orbA;
        Color orbB;
        Color orbC;
        bool glow = false;

        // color-mix percentages, 0..1
        float hoverMix = 0.18f;
        float activeMix = 0.28f;
        float cardMix = 0.10f;
        float ghostAlpha = 0.10f;
        float overlayHoverAlpha = 0.10f;
        float overlayPressAlpha = 0.16f;
        float rowAlpha = 0.05f;
        float washAlpha = 0.18f;

        // computed: finishTheme()
        Color ghost;
        Color overlayHover;
        Color overlayPress;
        Color rowIdle;
        Color rowIdleBottom;
    };

    const RoseTheme& theme();
    RoseThemeId themeId();
    const RoseTheme& themeById(RoseThemeId id);
    void applyRoseThemeId(RoseThemeId id);
    bool setRoseThemeId(RoseThemeId id);
    RoseThemeId themeIdFromKey(const String& key);
    const char* themeKey(RoseThemeId id);
    int roseThemeCount();
    RoseThemeId roseThemeAt(int index);

    Color withAlpha(const Color& color, float a);
    Color liftColor(const Color& color, float amount);
    Color mixWhite(const Color& color, float amount);
    Color mixBlack(const Color& color, float amount);
    Color hoverOf(const Color& color);
    Color pressOf(const Color& color);

}

#endif
