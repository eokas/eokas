#ifndef _EOKAS_ROSE_AXION_FIELD_EDITORS_H_
#define _EOKAS_ROSE_AXION_FIELD_EDITORS_H_

#include "./FieldTypes.h"
#include "./UiKit.h"

#include "UI/Main.h"

#include <functional>
#include <memory>
#include <vector>

namespace eokas::RoseAxion {

    bool optionKind(FieldKind kind);
    bool temporalKind(FieldKind kind);
    bool pathKind(FieldKind kind);
    bool pickerKind(FieldKind kind);
    bool boolFromText(const String& text);
    Color invertRgb(const Color& color);
    Color optionChipColor(const String& hex);
    float fieldTextPad(float scale);

    void bindEscape(UIWidget& widget, const std::function<void()>& onEscape);
    void centerFieldText(const std::shared_ptr<UIText>& text, float x, float width, float cellH, float scale);

    void styleToggle(UIToggle& toggle);
    void styleCellInput(
        UIInput& input,
        const Color& fill,
        float scale,
        const String& font,
        float fontPx,
        const std::function<void(const String&)>& writeClipboard,
        const std::function<String()>& readClipboard);

    template<typename T>
    void styleTemporalPicker(T& picker, float scale, const String& font, float fontSize)
    {
        const RoseTheme& t = theme();
        picker.shape.pivot = Vector2(0.0f, 0.0f);
        picker.uiScale = scale;
        picker.direction = UIDirection::Vertical;
        picker.setFont(font, fontSize);
        picker.background = t.field;
        picker.field = t.field;
        picker.ink = t.ink;
        picker.muted = t.mute;
        picker.accent = t.accent;
        picker.button = t.quiet;
        picker.buttonHover = hoverOf(t.quiet);
        picker.buttonPress = pressOf(t.quiet);
        picker.line = t.line;
    }

    class EscapeInput : public UIInput
    {
    public:
        std::function<void()> onEscape;
        bool handleKeyPress(const UIKeyEvent& event) override;
    };

    class EscapeColorPicker : public UIColorPicker
    {
    public:
        std::function<void()> onEscape;
        bool handleKeyPress(const UIKeyEvent& event) override;
    };

    std::shared_ptr<UIToggle> makeBoolToggle(float scale, const Vector2& hitSize, bool value);

    void attachOptionTags(
        UIWidget& hit,
        const Vector2& hitSize,
        float scale,
        FieldKind kind,
        const std::vector<FieldOption>& options,
        const std::vector<String>& labels,
        const String& font,
        float smallPx,
        const std::function<void(const String& label)>& onClose,
        const std::function<void()>& onClick);

    void attachColorValue(
        UIWidget& hit,
        const Vector2& hitSize,
        float scale,
        const String& shown,
        const String& font,
        float fontPx);

    void attachValueText(
        UIWidget& hit,
        const Vector2& hitSize,
        float scale,
        const String& shown,
        const String& font,
        float fontPx,
        const Color& color);

    std::shared_ptr<UIMenu> makeOptionMenu(
        const std::vector<FieldOption>& options,
        const std::vector<String>& selectedLabels,
        float scale,
        const String& font,
        float fontPx,
        float menuWidth,
        UIWidget* host,
        const std::function<void(const String& label)>& onPick,
        const std::function<void()>& onDismiss);

    void initTemporalParts(FieldKind kind, const String& shown, TemporalParts& parts);
    String formatTemporal(FieldKind kind, const TemporalParts& parts);

    struct DatePanelWidgets
    {
        std::shared_ptr<UIWidget> panel;
        UIWidget* focus = nullptr;
    };

    DatePanelWidgets makeDatePanel(
        FieldKind kind,
        const TemporalParts& initial,
        float scale,
        const String& font,
        float fontPx,
        float smallPx,
        const std::function<void()>& onClose,
        const std::function<void()>& onClear,
        const std::function<void(const String& stored)>& onCommit,
        const std::function<void()>& onEscape,
        const std::function<void(const TemporalParts&)>& onChanged);

    struct ColorPanelWidgets
    {
        std::shared_ptr<UIWidget> panel;
        UIColorPicker* picker = nullptr;
        UIWidget* focus = nullptr;
    };

    ColorPanelWidgets makeColorPanel(
        const Color& current,
        float scale,
        const String& font,
        float fontPx,
        float smallPx,
        const std::function<void(const String&)>& writeClipboard,
        const std::function<String()>& readClipboard,
        const std::function<void(const Color&)>& onCommit,
        const std::function<void()>& onClear,
        const std::function<void()>& onClose,
        const std::function<void()>& onEscape);

}

#endif
