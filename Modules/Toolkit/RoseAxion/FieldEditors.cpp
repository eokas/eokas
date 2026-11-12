#include "./FieldEditors.h"

#include "Base/Time.h"

#include <cmath>
#include <memory>
#include <utility>

namespace eokas::RoseAxion {

    bool optionKind(FieldKind kind)
    {
        return kind == FieldKind::SingleOption || kind == FieldKind::MultiOption;
    }

    bool temporalKind(FieldKind kind)
    {
        return kind == FieldKind::Date || kind == FieldKind::Time || kind == FieldKind::DateTime;
    }

    bool pathKind(FieldKind kind)
    {
        return kind == FieldKind::FilePath || kind == FieldKind::FolderPath;
    }

    bool pickerKind(FieldKind kind)
    {
        return kind == FieldKind::Boolean || kind == FieldKind::Color || temporalKind(kind) || pathKind(kind);
    }

    bool boolFromText(const String& text)
    {
        bool flag = false;
        return parseBool(text, flag) && flag;
    }

    Color invertRgb(const Color& color)
    {
        return Color(1.0f - color.r, 1.0f - color.g, 1.0f - color.b, 1.0f);
    }

    Color optionChipColor(const String& hex)
    {
        Color chip = theme().chip;
        parseColor(hex, chip);
        return chip;
    }

    float fieldTextPad(float scale)
    {
        return 6.0f * scale;
    }

    void bindEscape(UIWidget& widget, const std::function<void()>& onEscape)
    {
        if (dynamic_cast<UINumberPicker*>(&widget) != nullptr)
        {
            widget.onKeyPress = [onEscape](const UIKeyEvent& event)
            {
                if (event.key == UIKeyCode::Escape)
                {
                    onEscape();
                }
            };
        }
        for (const auto& child : widget.children())
        {
            if (child)
            {
                bindEscape(*child, onEscape);
            }
        }
    }

    void centerFieldText(const std::shared_ptr<UIText>& text, float x, float width, float cellH, float scale)
    {
        text->shape.pivot = Vector2(0.0f, 0.0f);
        text->layout();
        float h = text->shape.size.y;
        if (h < 1.0f)
        {
            h = 22.0f * scale;
        }
        float w = text->shape.size.x;
        if (w < 1.0f || w > width)
        {
            w = width;
        }
        setBox(text, Rect(x, (cellH - h) * 0.5f, w, h));
    }

    void styleToggle(UIToggle& toggle)
    {
        const RoseTheme& t = theme();
        toggle.off = t.quiet;
        toggle.on = t.accent;
        toggle.offHover = hoverOf(t.quiet);
        toggle.onHover = hoverOf(t.accent);
        toggle.offPressed = pressOf(t.quiet);
        toggle.onPressed = pressOf(t.accent);
        toggle.thumb = t.ink;
    }

    void styleCellInput(
        UIInput& input,
        const Color& fill,
        float scale,
        const String& font,
        float fontPx,
        const std::function<void(const String&)>& writeClipboard,
        const std::function<String()>& readClipboard)
    {
        const RoseTheme& t = theme();
        input.paddingX = fieldTextPad(scale);
        input.paddingY = 0.0f;
        input.background = fill;
        input.hoverFill = fill;
        input.border.color = Color(0.0f, 0.0f, 0.0f, 0.0f);
        input.border.thickness = 0.0f;
        input.caret.color = t.ink;
        input.selection = t.selection;
        if (UIText* label = input.label())
        {
            styleText(*label, font, fontPx, t.ink);
        }
        input.writeClipboard = writeClipboard;
        input.readClipboard = readClipboard;
    }

    bool EscapeInput::handleKeyPress(const UIKeyEvent& event)
    {
        if (event.key == UIKeyCode::Escape)
        {
            if (onEscape)
            {
                onEscape();
            }
            return true;
        }
        return UIInput::handleKeyPress(event);
    }

    bool EscapeColorPicker::handleKeyPress(const UIKeyEvent& event)
    {
        if (UIColorPicker::handleKeyPress(event))
        {
            return true;
        }
        if (event.key == UIKeyCode::Escape)
        {
            if (onEscape)
            {
                onEscape();
            }
            return true;
        }
        return false;
    }

    std::shared_ptr<UIToggle> makeBoolToggle(float scale, const Vector2& hitSize, bool value)
    {
        const float s = scale;
        const float pad = fieldTextPad(s);
        float toggleH = 22.0f * s;
        float toggleW = 40.0f * s;
        if (toggleH > hitSize.y - 4.0f * s)
        {
            toggleH = hitSize.y - 4.0f * s;
        }
        if (toggleH < 1.0f)
        {
            toggleH = 1.0f;
        }
        if (toggleW > hitSize.x - pad * 2.0f)
        {
            toggleW = hitSize.x - pad * 2.0f;
        }
        if (toggleW < toggleH)
        {
            toggleW = toggleH;
        }
        auto toggle = std::make_shared<UIToggle>();
        toggle->shape.pivot = Vector2(0.0f, 0.0f);
        setBox(toggle, Rect(pad, (hitSize.y - toggleH) * 0.5f, toggleW, toggleH));
        styleToggle(*toggle);
        toggle->value = value;
        return toggle;
    }

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
        const std::function<void()>& onClick)
    {
        (void)kind;
        const float s = scale;
        const Color clear(0.0f, 0.0f, 0.0f, 0.0f);
        auto tags = std::make_shared<UIList>();
        tags->direction = UIDirection::Horizontal;
        tags->padding = fieldTextPad(s);
        tags->spacing = 4.0f * s;
        tags->centerCross = true;
        tags->color = clear;
        tags->shape.pivot = Vector2(0.0f, 0.0f);
        setBox(tags, Rect(0.0f, 0.0f, hitSize.x, hitSize.y));
        for (const String& label : labels)
        {
            Color chip = optionChipColor(String::empty);
            for (const FieldOption& option : options)
            {
                if (option.label != label)
                {
                    continue;
                }
                chip = optionChipColor(option.color);
                break;
            }
            auto tag = std::make_shared<UITag>();
            tag->shape.pivot = Vector2(0.0f, 0.0f);
            tag->paddingX = 6.0f * s;
            tag->paddingY = 3.0f * s;
            tag->closeGap = 4.0f * s;
            tag->background = chip;
            tag->caption().text = label;
            styleText(tag->caption(), font, smallPx, invertRgb(chip));
            tag->onClose = [onClose, label]()
            {
                if (onClose)
                {
                    onClose(label);
                }
            };
            tag->onClick = onClick;
            tags->attachChild(tag);
        }
        hit.attachChild(tags);
    }

    void attachColorValue(
        UIWidget& hit,
        const Vector2& hitSize,
        float scale,
        const String& shown,
        const String& font,
        float fontPx)
    {
        Color chip;
        if (!parseColor(shown, chip))
        {
            return;
        }
        const float s = scale;
        const RoseTheme& t = theme();
        const float pad = fieldTextPad(s);
        float swatch = 14.0f * s;
        if (swatch > hitSize.y - 4.0f * s)
        {
            swatch = hitSize.y - 4.0f * s;
        }
        if (swatch < 1.0f)
        {
            swatch = 1.0f;
        }
        float borderW = 1.0f * s;
        if (borderW > swatch * 0.25f)
        {
            borderW = swatch * 0.25f;
        }
        auto border = std::make_shared<UIWidget>();
        border->pickable = false;
        border->shape.pivot = Vector2(0.0f, 0.0f);
        border->color = t.voidBg;
        setBox(border, Rect(pad, (hitSize.y - swatch) * 0.5f, swatch, swatch));
        auto swatchFill = std::make_shared<UIWidget>();
        swatchFill->pickable = false;
        swatchFill->shape.pivot = Vector2(0.0f, 0.0f);
        swatchFill->color = chip;
        float inner = swatch - borderW * 2.0f;
        if (inner < 1.0f)
        {
            inner = 1.0f;
        }
        setBox(swatchFill, Rect(borderW, borderW, inner, inner));
        border->attachChild(swatchFill);
        hit.attachChild(border);
        String hex = formatColor(chip);
        float labelX = pad + swatch + 6.0f * s;
        float labelW = hitSize.x - labelX - pad;
        if (labelW < 1.0f)
        {
            labelW = 1.0f;
        }
        auto label = makeText(hex, font, fontPx, chip, labelW, 22.0f * s);
        centerFieldText(label, labelX, labelW, hitSize.y, s);
        hit.attachChild(label);
    }

    void attachValueText(
        UIWidget& hit,
        const Vector2& hitSize,
        float scale,
        const String& shown,
        const String& font,
        float fontPx,
        const Color& color)
    {
        const float s = scale;
        const float pad = fieldTextPad(s);
        float width = hitSize.x - pad * 2.0f;
        if (width < 1.0f)
        {
            width = 1.0f;
        }
        auto label = makeText(shown, font, fontPx, color, width, 22.0f * s);
        centerFieldText(label, pad, width, hitSize.y, s);
        hit.attachChild(label);
    }

    std::shared_ptr<UIMenu> makeOptionMenu(
        const std::vector<FieldOption>& options,
        const std::vector<String>& selectedLabels,
        float scale,
        const String& font,
        float fontPx,
        float menuWidth,
        UIWidget* host,
        const std::function<void(const String& label)>& onPick,
        const std::function<void()>& onDismiss)
    {
        const float s = scale;
        const RoseTheme& t = theme();
        auto menu = std::make_shared<UIMenu>();
        menu->direction = UIDirection::Vertical;
        menu->padding = 4.0f * s;
        menu->spacing = 2.0f * s;
        menu->color = t.popup;
        menu->overlay = true;
        menu->shape.pivot = Vector2(0.0f, 0.0f);
        menu->host = host;
        if (options.empty())
        {
            auto item = std::make_shared<UIMenuItem>();
            item->setText("无可选选项");
            styleMenuItem(*item, font, fontPx);
            if (UIText* text = item->label())
            {
                styleText(*text, font, fontPx, t.mute);
            }
            menu->addItem(item);
        }
        for (const FieldOption& option : options)
        {
            auto item = std::make_shared<UIMenuItem>();
            item->paddingX = 10.0f * s;
            item->paddingY = 6.0f * s;
            bool on = selectedContains(selectedLabels, option.label);
            item->setText(option.label);
            styleMenuItem(*item, font, fontPx);
            item->selected = on;
            item->showChip = true;
            item->chip = optionChipColor(option.color);
            item->chipSize = 18.0f * s;
            item->chipGap = 8.0f * s;
            String label = option.label;
            item->onClick = [onPick, label]()
            {
                if (onPick)
                {
                    onPick(label);
                }
            };
            menu->addItem(item);
        }
        float pickH = 32.0f * s * static_cast<float>(options.empty() ? 1 : options.size()) + 8.0f * s;
        menu->shape.setBox(Vector2(0.0f, 0.0f), Vector2(menuWidth, pickH));
        UIMenu* menuRaw = menu.get();
        menu->onPointerPressAnywhere = [onDismiss, menuRaw](const UIPointerEvent&)
        {
            if (!menuRaw->visible && onDismiss)
            {
                onDismiss();
            }
        };
        return menu;
    }

    void initTemporalParts(FieldKind kind, const String& shown, TemporalParts& parts)
    {
        TimePoint now;
        parts.year = now.year();
        parts.month = now.month();
        parts.day = now.date();
        parts.hour = now.hour();
        parts.minute = now.minute();
        parts.second = now.second();
        if (kind == FieldKind::Date)
        {
            parseDate(shown, parts.year, parts.month, parts.day);
        }
        else if (kind == FieldKind::Time)
        {
            parseTime(shown, parts.hour, parts.minute, parts.second);
        }
        else
        {
            parseDateTime(shown, parts.year, parts.month, parts.day, parts.hour, parts.minute, parts.second);
        }
    }

    String formatTemporal(FieldKind kind, const TemporalParts& parts)
    {
        if (kind == FieldKind::Date)
        {
            return formatDate(parts.year, parts.month, parts.day);
        }
        if (kind == FieldKind::Time)
        {
            return formatTime(parts.hour, parts.minute, parts.second);
        }
        return formatDateTime(parts.year, parts.month, parts.day, parts.hour, parts.minute, parts.second);
    }

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
        const std::function<void(const TemporalParts&)>& onChanged)
    {
        const float s = scale;
        const RoseTheme& t = theme();
        const Color ink = t.ink;
        const Color mute = t.mute;
        const Color field = t.field;
        const Color quiet = t.quiet;
        const Color accent = t.accent;
        const Color clear(0.0f, 0.0f, 0.0f, 0.0f);
        auto parts = std::make_shared<TemporalParts>(initial);
        auto panel = std::make_shared<UIList>();
        panel->direction = UIDirection::Vertical;
        panel->padding = 16.0f * s;
        panel->spacing = 10.0f * s;
        panel->pickable = true;
        panel->color = t.raised;
        panel->shape.pivot = Vector2(0.0f, 0.0f);
        Vector2 pickerSize = UIDateTimePicker::preferredSize(s, true);
        const char* titleText = "日期时间选择";
        if (kind == FieldKind::Date)
        {
            pickerSize = UIDatePicker::preferredSize(s);
            titleText = "日期选择";
        }
        else if (kind == FieldKind::Time)
        {
            pickerSize = UITimePicker::preferredSize(s, true);
            titleText = "时间选择";
        }
        float panelW = pickerSize.x + 32.0f * s;
        float panelH = pickerSize.y + 116.0f * s;
        setBox(panel, Rect(0.0f, 0.0f, panelW, panelH));
        float innerW = panelW - 32.0f * s;
        auto head = std::make_shared<UIWidget>();
        head->pickable = false;
        head->shape.pivot = Vector2(0.0f, 0.0f);
        head->color = clear;
        setBox(head, Rect(0.0f, 0.0f, innerW, 32.0f * s));
        auto title = makeText(titleText, font, fontPx + 1.0f, ink, innerW - 76.0f * s, 28.0f * s);
        title->shape.pivot = Vector2(0.0f, 0.0f);
        setBox(title, Rect(0.0f, 2.0f * s, innerW - 76.0f * s, 28.0f * s));
        head->attachChild(title);
        auto close = makeButton("关闭", font, smallPx, 64.0f * s, 28.0f * s, quiet, ink);
        close->shape.setBox(Vector2(innerW - 64.0f * s, 2.0f * s), Vector2(64.0f * s, 28.0f * s));
        close->onClick = onClose;
        head->attachChild(close);
        panel->attachChild(head);
        std::shared_ptr<UIWidget> pickerWidget;
        std::function<void()> capturePicker;
        auto pickerKeyPress = [onEscape](const UIKeyEvent& event)
        {
            if (event.key == UIKeyCode::Escape && onEscape)
            {
                onEscape();
            }
        };
        if (kind == FieldKind::Date)
        {
            auto picker = std::make_shared<UIDatePicker>();
            styleTemporalPicker(*picker, s, font, smallPx);
            setBox(picker, Rect(0.0f, 0.0f, pickerSize.x, pickerSize.y));
            picker->setDate(parts->year, parts->month, parts->day);
            parts->year = picker->year();
            parts->month = picker->month();
            parts->day = picker->day();
            UIDatePicker* pickerRaw = picker.get();
            capturePicker = [parts, pickerRaw, onChanged]()
            {
                parts->year = pickerRaw->year();
                parts->month = pickerRaw->month();
                parts->day = pickerRaw->day();
                if (onChanged)
                {
                    onChanged(*parts);
                }
            };
            picker->onDateChanged = capturePicker;
            picker->onDateCommitted = capturePicker;
            picker->onKeyPress = pickerKeyPress;
            bindEscape(*picker, onEscape);
            pickerWidget = picker;
        }
        else if (kind == FieldKind::Time)
        {
            auto picker = std::make_shared<UITimePicker>();
            styleTemporalPicker(*picker, s, font, smallPx);
            picker->showSeconds = true;
            setBox(picker, Rect(0.0f, 0.0f, pickerSize.x, pickerSize.y));
            picker->setTime(parts->hour, parts->minute, parts->second);
            parts->hour = picker->hour();
            parts->minute = picker->minute();
            parts->second = picker->second();
            UITimePicker* pickerRaw = picker.get();
            capturePicker = [parts, pickerRaw, onChanged]()
            {
                parts->hour = pickerRaw->hour();
                parts->minute = pickerRaw->minute();
                parts->second = pickerRaw->second();
                if (onChanged)
                {
                    onChanged(*parts);
                }
            };
            picker->onTimeChanged = capturePicker;
            picker->onTimeCommitted = capturePicker;
            picker->onKeyPress = pickerKeyPress;
            bindEscape(*picker, onEscape);
            pickerWidget = picker;
        }
        else
        {
            auto picker = std::make_shared<UIDateTimePicker>();
            styleTemporalPicker(*picker, s, font, smallPx);
            picker->showSeconds = true;
            setBox(picker, Rect(0.0f, 0.0f, pickerSize.x, pickerSize.y));
            picker->setDateTime(parts->year, parts->month, parts->day, parts->hour, parts->minute, parts->second);
            parts->year = picker->year();
            parts->month = picker->month();
            parts->day = picker->day();
            parts->hour = picker->hour();
            parts->minute = picker->minute();
            parts->second = picker->second();
            UIDateTimePicker* pickerRaw = picker.get();
            capturePicker = [parts, pickerRaw, onChanged]()
            {
                parts->year = pickerRaw->year();
                parts->month = pickerRaw->month();
                parts->day = pickerRaw->day();
                parts->hour = pickerRaw->hour();
                parts->minute = pickerRaw->minute();
                parts->second = pickerRaw->second();
                if (onChanged)
                {
                    onChanged(*parts);
                }
            };
            picker->onDateTimeChanged = capturePicker;
            picker->onDateTimeCommitted = capturePicker;
            picker->onKeyPress = pickerKeyPress;
            bindEscape(*picker, onEscape);
            pickerWidget = picker;
        }
        panel->attachChild(pickerWidget);
        auto actions = std::make_shared<UIList>();
        actions->direction = UIDirection::Horizontal;
        actions->padding = 0.0f;
        actions->spacing = 8.0f * s;
        actions->color = clear;
        actions->shape.pivot = Vector2(0.0f, 0.0f);
        auto clearBtn = makeButton("清除", font, fontPx, 80.0f * s, 32.0f * s, quiet, ink);
        clearBtn->onClick = onClear;
        auto ok = makeButton("确定", font, fontPx, 80.0f * s, 32.0f * s, accent, t.inkOnAccent);
        ok->onClick = [kind, parts, capturePicker, onCommit]()
        {
            if (capturePicker)
            {
                capturePicker();
            }
            if (onCommit)
            {
                onCommit(formatTemporal(kind, *parts));
            }
        };
        close->onKeyPress = pickerKeyPress;
        clearBtn->onKeyPress = pickerKeyPress;
        ok->onKeyPress = pickerKeyPress;
        actions->attachChild(clearBtn);
        actions->attachChild(ok);
        panel->attachChild(actions);
        DatePanelWidgets result;
        result.panel = panel;
        result.focus = pickerWidget.get();
        return result;
    }

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
        const std::function<void()>& onEscape)
    {
        const float s = scale;
        const RoseTheme& t = theme();
        const Color ink = t.ink;
        const Color mute = t.mute;
        const Color field = t.field;
        const Color quiet = t.quiet;
        const Color accent = t.accent;
        const Color clear(0.0f, 0.0f, 0.0f, 0.0f);
        auto panel = std::make_shared<UIList>();
        panel->direction = UIDirection::Vertical;
        panel->padding = 16.0f * s;
        panel->spacing = 10.0f * s;
        panel->pickable = true;
        panel->color = t.raised;
        panel->shape.pivot = Vector2(0.0f, 0.0f);
        Vector2 pickerSize = UIColorPicker::preferredSize(s);
        float panelW = pickerSize.x + 32.0f * s;
        float panelH = pickerSize.y + 74.0f * s;
        setBox(panel, Rect(0.0f, 0.0f, panelW, panelH));
        float innerW = panelW - 32.0f * s;
        float btnW = 64.0f * s;
        float btnH = 28.0f * s;
        float btnGap = 8.0f * s;
        float headActions = btnW * 2.0f + btnGap + 12.0f * s;
        auto head = std::make_shared<UIWidget>();
        head->pickable = false;
        head->shape.pivot = Vector2(0.0f, 0.0f);
        head->color = clear;
        setBox(head, Rect(0.0f, 0.0f, innerW, 32.0f * s));
        auto title = makeText("颜色选择", font, fontPx + 1.0f, ink, innerW - headActions, 28.0f * s);
        title->shape.pivot = Vector2(0.0f, 0.0f);
        setBox(title, Rect(0.0f, 2.0f * s, innerW - headActions, 28.0f * s));
        head->attachChild(title);
        auto clearBtn = makeButton("清除", font, smallPx, btnW, btnH, quiet, ink);
        clearBtn->shape.setBox(Vector2(innerW - btnW * 2.0f - btnGap, 2.0f * s), Vector2(btnW, btnH));
        clearBtn->onClick = onClear;
        head->attachChild(clearBtn);
        auto close = makeButton("关闭", font, smallPx, btnW, btnH, quiet, ink);
        close->shape.setBox(Vector2(innerW - btnW, 2.0f * s), Vector2(btnW, btnH));
        close->onClick = onClose;
        head->attachChild(close);
        panel->attachChild(head);
        auto picker = std::make_shared<EscapeColorPicker>();
        picker->shape.pivot = Vector2(0.0f, 0.0f);
        picker->uiScale = s;
        picker->useAlpha = true;
        picker->setFont(font, smallPx);
        picker->background = field;
        picker->field = field;
        picker->ink = ink;
        picker->muted = mute;
        picker->accent = accent;
        picker->button = quiet;
        picker->buttonHover = hoverOf(quiet);
        picker->buttonPress = pressOf(quiet);
        picker->line = t.line;
        picker->writeClipboard = writeClipboard;
        picker->readClipboard = readClipboard;
        picker->setColor(current, true);
        setBox(picker, Rect(0.0f, 0.0f, pickerSize.x, pickerSize.y));
        picker->onColorCommitted = onCommit;
        picker->onCancelled = onClose;
        picker->onEscape = onEscape;
        picker->onKeyPress = [onEscape](const UIKeyEvent& event)
        {
            if (event.key == UIKeyCode::Escape && onEscape)
            {
                onEscape();
            }
        };
        bindEscape(*picker, onEscape);
        panel->attachChild(picker);
        close->onKeyPress = picker->onKeyPress;
        clearBtn->onKeyPress = picker->onKeyPress;
        ColorPanelWidgets result;
        result.panel = panel;
        result.picker = picker.get();
        result.focus = picker.get();
        return result;
    }

}
