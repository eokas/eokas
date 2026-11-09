#include "./Theme.h"
#include "./Prefs.h"

namespace eokas::RoseAxion {

    namespace {

        RoseThemeId gId = RoseThemeId::Light;

        RoseTheme finishTheme(RoseTheme t)
        {
            t.ghost = withAlpha(t.accentBright, t.ghostAlpha);
            t.overlayHover = withAlpha(t.ink, t.overlayHoverAlpha);
            t.overlayPress = Color(0.0f, 0.0f, 0.0f, t.overlayPressAlpha);
            t.rowIdle = withAlpha(t.accentBright, t.rowAlpha);
            t.rowIdleBottom = withAlpha(t.accentBright, t.rowAlpha * 0.40f);
            if (t.chip.a <= 0.001f)
            {
                t.chip = t.secondary;
            }
            if (t.ring.a <= 0.001f)
            {
                t.ring = t.accentBright;
            }
            if (t.inkOnAccent.a <= 0.001f)
            {
                t.inkOnAccent = t.ink;
            }
            if (t.selection.a <= 0.001f)
            {
                t.selection = withAlpha(t.accentBright, t.light ? 0.18f : 0.38f);
            }
            return t;
        }

        RoseTheme makeLight()
        {
            RoseTheme t;
            t.id = RoseThemeId::Light;
            t.key = "light";
            t.name = "浅色";
            t.blurb = "云母浅底、柔分层、雾蓝";
            t.light = true;
            t.voidBg = Color(0.902f, 0.910f, 0.933f, 1.0f);
            t.canvas = Color(0.953f, 0.957f, 0.969f, 1.0f);
            t.paper = Color(1.000f, 1.000f, 1.000f, 1.0f);
            t.bar = Color(0.973f, 0.976f, 0.984f, 1.0f);
            t.header = Color(0.941f, 0.949f, 0.961f, 1.0f);
            t.raised = Color(0.973f, 0.976f, 0.984f, 1.0f);
            t.popup = Color(0.980f, 0.984f, 0.992f, 1.0f);
            t.field = Color(0.965f, 0.969f, 0.980f, 1.0f);
            t.quiet = Color(0.910f, 0.918f, 0.933f, 1.0f);
            t.navIdle = Color(0.953f, 0.957f, 0.969f, 1.0f);
            t.navActive = Color(1.000f, 1.000f, 1.000f, 1.0f);
            t.selected = Color(0.890f, 0.918f, 0.953f, 1.0f);
            t.selectedCell = Color(0.000f, 0.404f, 0.753f, 0.16f);
            t.shade = Color(0.00f, 0.00f, 0.00f, 0.18f);
            t.ink = Color(0.110f, 0.110f, 0.118f, 1.0f);
            t.mute = Color(0.369f, 0.380f, 0.408f, 1.0f);
            t.inkOnAccent = Color(1.00f, 1.00f, 1.00f, 1.0f);
            t.url = Color(0.000f, 0.404f, 0.753f, 1.0f);
            t.line = Color(0.824f, 0.835f, 0.863f, 1.0f);
            t.hair = Color(0.00f, 0.00f, 0.00f, 0.08f);
            t.gridLine = Color(0.804f, 0.816f, 0.847f, 1.0f);
            t.accent = Color(0.000f, 0.404f, 0.753f, 1.0f);
            t.accentBright = Color(0.227f, 0.624f, 0.878f, 1.0f);
            t.secondary = Color(0.357f, 0.404f, 0.502f, 1.0f);
            t.warn = Color(0.604f, 0.357f, 0.000f, 1.0f);
            t.danger = Color(0.769f, 0.169f, 0.110f, 1.0f);
            t.chip = t.secondary;
            t.ring = t.accent;
            t.heroTl = Color(0.933f, 0.949f, 0.973f, 1.0f);
            t.heroTr = Color(0.965f, 0.973f, 0.988f, 1.0f);
            t.heroBr = Color(1.000f, 1.000f, 1.000f, 1.0f);
            t.heroBl = Color(0.941f, 0.953f, 0.973f, 1.0f);
            t.cardTop = Color(1.000f, 1.000f, 1.000f, 1.0f);
            t.cardBottom = Color(0.969f, 0.973f, 0.984f, 1.0f);
            t.sectionTop = Color(1.000f, 1.000f, 1.000f, 1.0f);
            t.sectionBottom = Color(0.957f, 0.961f, 0.976f, 1.0f);
            t.orbA = Color(0.23f, 0.62f, 0.88f, 0.05f);
            t.orbB = Color(0.36f, 0.40f, 0.50f, 0.03f);
            t.orbC = Color(1.00f, 1.00f, 1.00f, 0.00f);
            t.glow = false;
            t.hoverMix = 0.04f;
            t.activeMix = 0.08f;
            t.cardMix = 0.03f;
            t.ghostAlpha = 0.10f;
            t.overlayHoverAlpha = 0.05f;
            t.overlayPressAlpha = 0.08f;
            t.rowAlpha = 0.06f;
            t.washAlpha = 0.14f;
            return finishTheme(t);
        }

        RoseTheme makeBlack()
        {
            RoseTheme t;
            t.id = RoseThemeId::Black;
            t.key = "black";
            t.name = "黑色";
            t.blurb = "OLED 黑底、高对比、冰青描边";
            t.light = false;
            t.voidBg = Color(0.000f, 0.000f, 0.000f, 1.0f);
            t.canvas = Color(0.035f, 0.036f, 0.040f, 1.0f);
            t.paper = Color(0.078f, 0.080f, 0.088f, 1.0f);
            t.bar = Color(0.000f, 0.000f, 0.000f, 1.0f);
            t.header = Color(0.090f, 0.092f, 0.100f, 1.0f);
            t.raised = Color(0.110f, 0.112f, 0.122f, 1.0f);
            t.popup = Color(0.070f, 0.072f, 0.080f, 1.0f);
            t.field = Color(0.018f, 0.018f, 0.022f, 1.0f);
            t.quiet = Color(0.145f, 0.150f, 0.165f, 1.0f);
            t.navIdle = Color(0.055f, 0.056f, 0.062f, 1.0f);
            t.navActive = Color(0.22f, 0.62f, 0.92f, 1.0f);
            t.selected = Color(0.22f, 0.62f, 0.92f, 1.0f);
            t.selectedCell = Color(0.42f, 0.86f, 1.00f, 0.18f);
            t.shade = Color(0.00f, 0.00f, 0.00f, 0.72f);
            t.ink = Color(0.96f, 0.97f, 0.98f, 1.0f);
            t.mute = Color(0.58f, 0.62f, 0.68f, 1.0f);
            t.inkOnAccent = Color(0.02f, 0.04f, 0.07f, 1.0f);
            t.url = Color(0.55f, 0.88f, 1.00f, 1.0f);
            t.line = Color(0.000f, 0.000f, 0.000f, 1.0f);
            t.hair = Color(0.80f, 0.92f, 1.00f, 0.10f);
            t.gridLine = Color(0.16f, 0.17f, 0.20f, 1.0f);
            t.accent = Color(0.22f, 0.62f, 0.92f, 1.0f);
            t.accentBright = Color(0.48f, 0.88f, 1.00f, 1.0f);
            t.secondary = Color(0.70f, 0.78f, 0.88f, 1.0f);
            t.warn = Color(0.96f, 0.78f, 0.22f, 1.0f);
            t.danger = Color(0.92f, 0.28f, 0.34f, 1.0f);
            t.chip = t.secondary;
            t.ring = t.accentBright;
            t.heroTl = Color(0.04f, 0.07f, 0.10f, 1.0f);
            t.heroTr = Color(0.03f, 0.04f, 0.08f, 1.0f);
            t.heroBr = Color(0.00f, 0.00f, 0.00f, 1.0f);
            t.heroBl = Color(0.00f, 0.00f, 0.00f, 1.0f);
            t.cardTop = Color(0.09f, 0.09f, 0.10f, 1.0f);
            t.cardBottom = Color(0.04f, 0.04f, 0.05f, 1.0f);
            t.sectionTop = Color(0.07f, 0.07f, 0.08f, 1.0f);
            t.sectionBottom = Color(0.03f, 0.03f, 0.04f, 1.0f);
            t.orbA = Color(0.48f, 0.88f, 1.00f, 0.04f);
            t.orbB = Color(0.22f, 0.62f, 0.92f, 0.03f);
            t.orbC = Color(1.00f, 1.00f, 1.00f, 0.00f);
            t.glow = false;
            t.hoverMix = 0.12f;
            t.activeMix = 0.30f;
            t.cardMix = 0.10f;
            t.ghostAlpha = 0.12f;
            t.overlayHoverAlpha = 0.10f;
            t.overlayPressAlpha = 0.22f;
            t.rowAlpha = 0.06f;
            t.washAlpha = 0.16f;
            return finishTheme(t);
        }

        RoseTheme makeUnreal()
        {
            RoseTheme t;
            t.id = RoseThemeId::Unreal;
            t.key = "unreal";
            t.name = "UE5";
            t.blurb = "启动器近黑底与虚幻金";
            t.light = false;
            t.voidBg = Color(10.0f / 255.0f, 10.0f / 255.0f, 10.0f / 255.0f, 1.0f);
            t.canvas = Color(18.0f / 255.0f, 18.0f / 255.0f, 18.0f / 255.0f, 1.0f);
            t.paper = Color(28.0f / 255.0f, 28.0f / 255.0f, 28.0f / 255.0f, 1.0f);
            t.bar = Color(14.0f / 255.0f, 14.0f / 255.0f, 14.0f / 255.0f, 1.0f);
            t.header = Color(22.0f / 255.0f, 22.0f / 255.0f, 22.0f / 255.0f, 1.0f);
            t.raised = Color(42.0f / 255.0f, 42.0f / 255.0f, 42.0f / 255.0f, 1.0f);
            t.popup = Color(28.0f / 255.0f, 28.0f / 255.0f, 28.0f / 255.0f, 1.0f);
            t.field = Color(32.0f / 255.0f, 32.0f / 255.0f, 32.0f / 255.0f, 1.0f);
            t.quiet = Color(49.0f / 255.0f, 49.0f / 255.0f, 49.0f / 255.0f, 1.0f);
            t.navIdle = t.bar;
            t.navActive = Color(42.0f / 255.0f, 42.0f / 255.0f, 42.0f / 255.0f, 1.0f);
            t.selected = Color(42.0f / 255.0f, 42.0f / 255.0f, 42.0f / 255.0f, 1.0f);
            t.selectedCell = Color(51.0f / 255.0f, 51.0f / 255.0f, 51.0f / 255.0f, 1.0f);
            t.selection = Color(245.0f / 255.0f, 197.0f / 255.0f, 24.0f / 255.0f, 0.38f);
            t.shade = Color(0.00f, 0.00f, 0.00f, 0.72f);
            t.ink = Color(230.0f / 255.0f, 230.0f / 255.0f, 230.0f / 255.0f, 1.0f);
            t.mute = Color(140.0f / 255.0f, 140.0f / 255.0f, 140.0f / 255.0f, 1.0f);
            t.inkOnAccent = Color(18.0f / 255.0f, 14.0f / 255.0f, 4.0f / 255.0f, 1.0f);
            t.url = Color(245.0f / 255.0f, 197.0f / 255.0f, 24.0f / 255.0f, 1.0f);
            t.line = Color(28.0f / 255.0f, 28.0f / 255.0f, 28.0f / 255.0f, 1.0f);
            t.hair = Color(1.00f, 1.00f, 1.00f, 0.06f);
            t.gridLine = Color(42.0f / 255.0f, 42.0f / 255.0f, 42.0f / 255.0f, 1.0f);
            t.accent = Color(232.0f / 255.0f, 180.0f / 255.0f, 10.0f / 255.0f, 1.0f);
            t.accentBright = Color(245.0f / 255.0f, 197.0f / 255.0f, 24.0f / 255.0f, 1.0f);
            t.secondary = Color(74.0f / 255.0f, 106.0f / 255.0f, 140.0f / 255.0f, 1.0f);
            t.warn = Color(232.0f / 255.0f, 155.0f / 255.0f, 44.0f / 255.0f, 1.0f);
            t.danger = Color(0.86f, 0.19f, 0.13f, 1.0f);
            t.chip = t.secondary;
            t.ring = t.accentBright;
            t.heroTl = Color(36.0f / 255.0f, 30.0f / 255.0f, 12.0f / 255.0f, 1.0f);
            t.heroTr = Color(22.0f / 255.0f, 22.0f / 255.0f, 22.0f / 255.0f, 1.0f);
            t.heroBr = t.canvas;
            t.heroBl = t.canvas;
            t.cardTop = Color(36.0f / 255.0f, 36.0f / 255.0f, 36.0f / 255.0f, 1.0f);
            t.cardBottom = Color(22.0f / 255.0f, 22.0f / 255.0f, 22.0f / 255.0f, 1.0f);
            t.sectionTop = Color(28.0f / 255.0f, 28.0f / 255.0f, 28.0f / 255.0f, 1.0f);
            t.sectionBottom = Color(18.0f / 255.0f, 18.0f / 255.0f, 18.0f / 255.0f, 1.0f);
            t.orbA = Color(245.0f / 255.0f, 197.0f / 255.0f, 24.0f / 255.0f, 0.05f);
            t.orbB = Color(74.0f / 255.0f, 106.0f / 255.0f, 140.0f / 255.0f, 0.04f);
            t.orbC = Color(1.00f, 1.00f, 1.00f, 0.00f);
            t.glow = false;
            t.hoverMix = 0.10f;
            t.activeMix = 0.22f;
            t.cardMix = 0.08f;
            t.ghostAlpha = 0.14f;
            t.overlayHoverAlpha = 0.12f;
            t.overlayPressAlpha = 0.22f;
            t.rowAlpha = 0.07f;
            t.washAlpha = 0.18f;
            return finishTheme(t);
        }

        const RoseTheme kLight = makeLight();
        const RoseTheme kBlack = makeBlack();
        const RoseTheme kUnreal = makeUnreal();

    }

    const RoseTheme& themeById(RoseThemeId id)
    {
        if (id == RoseThemeId::Black)
        {
            return kBlack;
        }
        if (id == RoseThemeId::Unreal)
        {
            return kUnreal;
        }
        return kLight;
    }

    const RoseTheme& theme()
    {
        return themeById(gId);
    }

    RoseThemeId themeId()
    {
        return gId;
    }

    const char* themeKey(RoseThemeId id)
    {
        return themeById(id).key;
    }

    RoseThemeId themeIdFromKey(const String& key)
    {
        if (key == "black" || key == "neon")
        {
            return RoseThemeId::Black;
        }
        if (key == "unreal" || key == "graphite")
        {
            return RoseThemeId::Unreal;
        }
        if (key == "light" || key == "night")
        {
            return RoseThemeId::Light;
        }
        return RoseThemeId::Light;
    }

    void applyRoseThemeId(RoseThemeId id)
    {
        gId = id;
    }

    bool setRoseThemeId(RoseThemeId id)
    {
        if (gId == id)
        {
            return false;
        }
        gId = id;
        savePrefs();
        return true;
    }

    int roseThemeCount()
    {
        return 3;
    }

    RoseThemeId roseThemeAt(int index)
    {
        if (index == 1)
        {
            return RoseThemeId::Black;
        }
        if (index == 2)
        {
            return RoseThemeId::Unreal;
        }
        return RoseThemeId::Light;
    }

    Color withAlpha(const Color& color, float a)
    {
        return Color(color.r, color.g, color.b, a);
    }

    Color liftColor(const Color& color, float amount)
    {
        return Color(
            color.r + (1.0f - color.r) * amount,
            color.g + (1.0f - color.g) * amount,
            color.b + (1.0f - color.b) * amount,
            color.a);
    }

    Color mixWhite(const Color& color, float amount)
    {
        return liftColor(color, amount);
    }

    Color mixBlack(const Color& color, float amount)
    {
        const float keep = 1.0f - amount;
        return Color(color.r * keep, color.g * keep, color.b * keep, color.a);
    }

    Color hoverOf(const Color& color)
    {
        if (theme().light)
        {
            return mixBlack(color, theme().hoverMix);
        }
        return mixWhite(color, theme().hoverMix);
    }

    Color pressOf(const Color& color)
    {
        return mixBlack(color, theme().activeMix);
    }

}
