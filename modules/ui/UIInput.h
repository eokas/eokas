#ifndef _EOKAS_UI_INPUT_H_
#define _EOKAS_UI_INPUT_H_

#include "UIText.h"

namespace eokas
{
    class UIInput : public UIWidget
    {
    public:
        String text;
        String placeholder;
        float paddingX = 8.0f;
        float paddingY = 6.0f;
        float borderThickness = 1.0f;
        size_t maxLength = 0;
        bool readOnly = false;
        bool multiline = false;
        Color background { 0.14f, 0.15f, 0.18f, 1.0f };
        Color hoverColor { 0.20f, 0.22f, 0.28f, 1.0f };
        Color borderColor { 0.32f, 0.44f, 0.68f, 1.0f };
        Color placeholderColor { 0.55f, 0.58f, 0.64f, 1.0f };
        Color caretColor { 0.92f, 0.92f, 0.94f, 1.0f };
        Color selectionColor { 0.32f, 0.44f, 0.68f, 1.0f };
        std::function<void(const String&)> onValueChanged;
        std::function<void(const String&)> onSubmit;
        std::function<void(const String&)> writeClipboard;
        std::function<String()> readClipboard;

        UIInput();
        void setText(const String& value);
        UIText* label() const;
        bool acceptsKeyFocus() const override { return true; }
        void render(UIShape& shape) override;
        void triggerPointerPress() override;
        void triggerPointerDrag(float x, float y, int button) override;
        void triggerPointerRelease() override;
        void triggerFocus() override;
        void triggerBlur() override;
        void triggerChar(uint32_t codepoint) override;
        void triggerKey(UIKey key, const UIKeyMods& mods) override;
        void resetPointerState() override;

    private:
        std::shared_ptr<UIText> mLabel;
        size_t mCaret = 0;
        size_t mAnchor = 0;
        float mScroll = 0.0f;
        float mScrollY = 0.0f;
        float mPreferredX = -1.0f;
        bool mPointerSelecting = false;
        bool mCaretPlaced = false;
        std::chrono::steady_clock::time_point mCaretBlinkAnchor;

        UIFont* activeFont() const;
        float textScale(UIFont* font) const;
        float lineBox(UIFont* font) const;
        float advanceBetween(const String& value, size_t begin, size_t end) const;
        size_t indexOnLine(const String& value, size_t begin, size_t end, float localX) const;
        Rect contentRect() const;
        String sanitize(const String& value) const;
        bool hasSelection() const;
        String selectedText() const;
        void clampCaret();
        void assignText(const String& next, size_t caret, size_t anchor, bool notify);
        void insertText(const String& raw);
        void deleteCaret(bool backspace);
        void moveCaret(size_t index, bool shift);
        void moveCaretBy(int direction, bool shift);
        void moveCaretVertically(int direction, bool shift);
        size_t stepCodepoint(size_t index, int direction) const;
        void ensureCaretVisible();
        void resetCaretBlink();
        float offsetOf(size_t index) const;
        size_t indexAt(float x, float y) const;
        void drawBorder(UIShape& shape) const;
        void drawSelection(UIShape& shape, const Rect& content) const;
        void drawGlyphRun(UIShape& shape, const String& value, const Color& color, const Rect& content, float scroll) const;
        void paintRange(UIShape& shape, const String& value, size_t begin, size_t end, const Color& color, const Rect& content, float baseline, float scrollX) const;
        void drawCaret(UIShape& shape, const Rect& content) const;
    };
}

#endif//_EOKAS_UI_INPUT_H_
