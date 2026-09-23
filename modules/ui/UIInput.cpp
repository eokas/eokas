#include "UIInput.h"
#include "UIFont.h"
#include <chrono>
#include <cmath>
#include <vector>

namespace eokas
{
    namespace
    {
        float snap(float v)
        {
            return floorf(v + 0.5f);
        }

        size_t codepointCount(const String& value)
        {
            size_t index = 0;
            size_t count = 0;
            uint32_t codepoint = 0;
            while (index < value.length())
            {
                if (!UIFont::nextUtf8(value.cstr(), value.length(), index, codepoint))
                {
                    break;
                }
                count += 1;
            }
            return count;
        }

        String codepointPrefix(const String& value, size_t count)
        {
            size_t index = 0;
            size_t seen = 0;
            uint32_t codepoint = 0;
            while (index < value.length() && seen < count)
            {
                if (!UIFont::nextUtf8(value.cstr(), value.length(), index, codepoint))
                {
                    break;
                }
                seen += 1;
            }
            return value.substr(0, index);
        }

        bool clipSpan(float& pos, float& size, float& uvPos, float& uvSize, float minPos, float maxPos)
        {
            if (size <= 0.0f || pos + size <= minPos || pos >= maxPos)
            {
                return false;
            }
            if (pos < minPos)
            {
                float cut = minPos - pos;
                float t = cut / size;
                pos = minPos;
                size -= cut;
                uvPos += uvSize * t;
                uvSize -= uvSize * t;
            }
            if (pos + size > maxPos)
            {
                float cut = pos + size - maxPos;
                float t = cut / size;
                size -= cut;
                uvSize -= uvSize * t;
            }
            return size > 0.0f && uvSize > 0.0f;
        }

        struct TextLine
        {
            size_t begin = 0;
            size_t end = 0;
        };

        std::vector<TextLine> splitLines(const String& value)
        {
            std::vector<TextLine> lines;
            size_t start = 0;
            size_t size = value.length();
            const char* data = value.cstr();
            for (size_t i = 0; i < size; ++i)
            {
                if (data[i] == '\n')
                {
                    lines.push_back(TextLine{ start, i });
                    start = i + 1;
                }
            }
            lines.push_back(TextLine{ start, size });
            return lines;
        }

        size_t lineOf(const std::vector<TextLine>& lines, size_t index)
        {
            for (size_t i = 0; i < lines.size(); ++i)
            {
                if (i + 1 == lines.size() || index < lines[i + 1].begin)
                {
                    return i;
                }
            }
            return 0;
        }
    }

    UIInput::UIInput()
    {
        auto created = std::make_shared<UIText>();
        created->visible = false;
        mLabel = created;
        children.push_back(created);
        mCaretBlinkAnchor = std::chrono::steady_clock::now();
    }

    void UIInput::setText(const String& value)
    {
        String next = this->sanitize(value);
        if (maxLength > 0 && codepointCount(next) > maxLength)
        {
            next = codepointPrefix(next, maxLength);
        }
        size_t end = next.length();
        this->assignText(next, end, end, true);
    }

    UIText* UIInput::label() const
    {
        return mLabel.get();
    }

    UIFont* UIInput::activeFont() const
    {
        UIText* t = this->label();
        if (t == nullptr || t->font == nullptr || !t->font->isOpen())
        {
            return nullptr;
        }
        return t->font;
    }

    float UIInput::textScale(UIFont* font) const
    {
        UIText* t = this->label();
        float size = t != nullptr ? t->fontSize : UIText::kDefaultFontSize;
        float bake = (float)font->pixelSize();
        if (bake <= 0.0f)
        {
            return 1.0f;
        }
        return (size > 0.0f ? size : bake) / bake;
    }

    float UIInput::lineBox(UIFont* font) const
    {
        float scale = this->textScale(font);
        float height = font->lineHeight() * scale;
        if (height < 1.0f)
        {
            height = (font->ascender() - font->descender()) * scale;
        }
        if (height < 1.0f)
        {
            height = 1.0f;
        }
        return height;
    }

    float UIInput::advanceBetween(const String& value, size_t begin, size_t end) const
    {
        UIFont* font = this->activeFont();
        if (font == nullptr || begin >= end)
        {
            return 0.0f;
        }
        if (end > value.length())
        {
            end = value.length();
        }
        if (begin > end)
        {
            return 0.0f;
        }
        float scale = this->textScale(font);
        float width = 0.0f;
        size_t i = begin;
        while (i < end)
        {
            uint32_t codepoint = 0;
            if (!UIFont::nextUtf8(value.cstr(), value.length(), i, codepoint) || i > end)
            {
                break;
            }
            width += font->glyph(codepoint).advance * scale;
        }
        return width;
    }

    size_t UIInput::indexOnLine(const String& value, size_t begin, size_t end, float localX) const
    {
        if (begin > value.length())
        {
            return value.length();
        }
        if (end > value.length())
        {
            end = value.length();
        }
        if (begin > end)
        {
            begin = end;
        }
        UIFont* font = this->activeFont();
        if (font == nullptr || localX <= 0.0f || begin >= end)
        {
            return begin;
        }
        float scale = this->textScale(font);
        float width = 0.0f;
        size_t i = begin;
        while (i < end)
        {
            size_t before = i;
            uint32_t codepoint = 0;
            if (!UIFont::nextUtf8(value.cstr(), value.length(), i, codepoint) || i > end)
            {
                break;
            }
            float advance = font->glyph(codepoint).advance * scale;
            if (localX < width + advance * 0.5f)
            {
                return before;
            }
            width += advance;
        }
        return end;
    }

    Rect UIInput::contentRect() const
    {
        float insetX = paddingX + borderThickness;
        float insetY = paddingY + borderThickness;
        float w = rect.width - insetX * 2.0f;
        float h = rect.height - insetY * 2.0f;
        if (w < 0.0f)
        {
            w = 0.0f;
        }
        if (h < 0.0f)
        {
            h = 0.0f;
        }
        return Rect(rect.x + insetX, rect.y + insetY, w, h);
    }

    String UIInput::sanitize(const String& value) const
    {
        String out;
        size_t index = 0;
        const char* data = value.cstr();
        size_t size = value.length();
        while (index < size)
        {
            size_t before = index;
            uint32_t codepoint = 0;
            if (!UIFont::nextUtf8(data, size, index, codepoint))
            {
                continue;
            }
            if (codepoint == '\n')
            {
                if (multiline)
                {
                    out.append(value.substr(before, index - before));
                }
                continue;
            }
            if (codepoint == '\r')
            {
                if (multiline && (index >= size || data[index] != '\n'))
                {
                    out.append("\n");
                }
                continue;
            }
            if (codepoint < 32 || codepoint == 127)
            {
                continue;
            }
            out.append(value.substr(before, index - before));
        }
        return out;
    }

    bool UIInput::hasSelection() const
    {
        return mAnchor != mCaret;
    }

    String UIInput::selectedText() const
    {
        size_t a = Math::min_s(mAnchor, mCaret);
        size_t b = Math::max_s(mAnchor, mCaret);
        if (a >= b)
        {
            return String();
        }
        return text.substr(a, b - a);
    }

    void UIInput::clampCaret()
    {
        if (mCaret > text.length())
        {
            mCaret = text.length();
        }
        if (mAnchor > text.length())
        {
            mAnchor = text.length();
        }
    }

    void UIInput::resetCaretBlink()
    {
        mCaretBlinkAnchor = std::chrono::steady_clock::now();
    }

    void UIInput::assignText(const String& next, size_t caret, size_t anchor, bool notify)
    {
        if (caret > next.length())
        {
            caret = next.length();
        }
        if (anchor > next.length())
        {
            anchor = next.length();
        }
        if (next == text && caret == mCaret && anchor == mAnchor)
        {
            return;
        }
        bool changed = next != text;
        text = next;
        mCaret = caret;
        mAnchor = anchor;
        mPreferredX = -1.0f;
        this->ensureCaretVisible();
        this->resetCaretBlink();
        if (changed && notify && onValueChanged)
        {
            onValueChanged(text);
        }
    }

    void UIInput::insertText(const String& raw)
    {
        if (readOnly)
        {
            return;
        }
        this->clampCaret();
        size_t a = Math::min_s(mAnchor, mCaret);
        size_t b = Math::max_s(mAnchor, mCaret);
        String chunk = this->sanitize(raw);
        String left = text.substr(0, a);
        String right = text.substr(b);
        if (maxLength > 0)
        {
            size_t keep = codepointCount(left) + codepointCount(right);
            if (keep >= maxLength)
            {
                chunk.clear();
            }
            else if (keep + codepointCount(chunk) > maxLength)
            {
                chunk = codepointPrefix(chunk, maxLength - keep);
            }
        }
        if (chunk.isEmpty() && a == b)
        {
            return;
        }
        String next = left + chunk + right;
        size_t caret = a + chunk.length();
        this->assignText(next, caret, caret, true);
    }

    void UIInput::deleteCaret(bool backspace)
    {
        if (readOnly)
        {
            return;
        }
        this->clampCaret();
        if (this->hasSelection())
        {
            this->insertText(String());
            return;
        }
        mAnchor = this->stepCodepoint(mCaret, backspace ? -1 : 1);
        if (mAnchor == mCaret)
        {
            return;
        }
        this->insertText(String());
    }

    void UIInput::moveCaret(size_t index, bool shift)
    {
        this->clampCaret();
        if (index > text.length())
        {
            index = text.length();
        }
        mCaret = index;
        if (!shift)
        {
            mAnchor = mCaret;
        }
        mPreferredX = -1.0f;
        this->ensureCaretVisible();
        this->resetCaretBlink();
    }

    void UIInput::moveCaretBy(int direction, bool shift)
    {
        this->clampCaret();
        if (!shift && this->hasSelection())
        {
            size_t edge = direction < 0 ? Math::min_s(mAnchor, mCaret) : Math::max_s(mAnchor, mCaret);
            this->moveCaret(edge, false);
            return;
        }
        this->moveCaret(this->stepCodepoint(mCaret, direction), shift);
    }

    void UIInput::moveCaretVertically(int direction, bool shift)
    {
        if (!multiline || direction == 0)
        {
            return;
        }
        this->clampCaret();
        std::vector<TextLine> lines = splitLines(text);
        size_t line = lineOf(lines, mCaret);
        size_t caret = mCaret;
        if (caret < lines[line].begin)
        {
            caret = lines[line].begin;
        }
        if (caret > lines[line].end)
        {
            caret = lines[line].end;
        }
        if (mPreferredX < 0.0f)
        {
            mPreferredX = this->advanceBetween(text, lines[line].begin, caret);
        }
        int next = (int)line + direction;
        size_t index = 0;
        if (next < 0)
        {
            index = 0;
        }
        else if (next >= (int)lines.size())
        {
            index = text.length();
        }
        else
        {
            const TextLine& target = lines[(size_t)next];
            index = this->indexOnLine(text, target.begin, target.end, mPreferredX);
        }
        float column = mPreferredX;
        this->moveCaret(index, shift);
        mPreferredX = column;
    }

    size_t UIInput::stepCodepoint(size_t index, int direction) const
    {
        if (direction < 0)
        {
            if (index == 0)
            {
                return 0;
            }
            size_t i = index;
            do
            {
                i -= 1;
            } while (i > 0 && ((unsigned char)text.at(i) & 0xC0) == 0x80);
            return i;
        }
        if (index >= text.length())
        {
            return text.length();
        }
        uint32_t codepoint = 0;
        size_t next = index;
        if (!UIFont::nextUtf8(text.cstr(), text.length(), next, codepoint))
        {
            return Math::min_s(index + 1, text.length());
        }
        return next;
    }

    float UIInput::offsetOf(size_t index) const
    {
        if (index > text.length())
        {
            index = text.length();
        }
        if (!multiline)
        {
            return this->advanceBetween(text, 0, index);
        }
        std::vector<TextLine> lines = splitLines(text);
        size_t line = lineOf(lines, index);
        size_t from = lines[line].begin;
        size_t to = index;
        if (to < from)
        {
            to = from;
        }
        if (to > lines[line].end)
        {
            to = lines[line].end;
        }
        return this->advanceBetween(text, from, to);
    }

    void UIInput::ensureCaretVisible()
    {
        Rect content = this->contentRect();
        if (!multiline)
        {
            mScrollY = 0.0f;
            float caret = this->offsetOf(mCaret);
            if (caret < mScroll)
            {
                mScroll = caret;
            }
            else if (caret > mScroll + content.width)
            {
                mScroll = caret - content.width;
            }
            float total = this->offsetOf(text.length());
            float maxScroll = total > content.width ? total - content.width : 0.0f;
            if (mScroll > maxScroll)
            {
                mScroll = maxScroll;
            }
            if (mScroll < 0.0f)
            {
                mScroll = 0.0f;
            }
            return;
        }

        std::vector<TextLine> lines = splitLines(text);
        size_t line = lineOf(lines, mCaret);
        float caretX = this->offsetOf(mCaret);
        if (caretX < mScroll)
        {
            mScroll = caretX;
        }
        else if (caretX + 1.0f > mScroll + content.width)
        {
            mScroll = caretX + 1.0f - content.width;
        }
        float widest = 0.0f;
        for (const TextLine& item : lines)
        {
            float width = this->advanceBetween(text, item.begin, item.end);
            if (width > widest)
            {
                widest = width;
            }
        }
        float maxScroll = widest > content.width ? widest - content.width : 0.0f;
        if (mScroll > maxScroll)
        {
            mScroll = maxScroll;
        }
        if (mScroll < 0.0f)
        {
            mScroll = 0.0f;
        }

        UIFont* font = this->activeFont();
        if (font == nullptr)
        {
            mScrollY = 0.0f;
            return;
        }
        float lineH = this->lineBox(font);
        float caretY = (float)line * lineH;
        if (caretY < mScrollY)
        {
            mScrollY = caretY;
        }
        else if (caretY + lineH > mScrollY + content.height)
        {
            mScrollY = caretY + lineH - content.height;
        }
        float totalH = (float)lines.size() * lineH;
        float maxScrollY = totalH > content.height ? totalH - content.height : 0.0f;
        if (mScrollY > maxScrollY)
        {
            mScrollY = maxScrollY;
        }
        if (mScrollY < 0.0f)
        {
            mScrollY = 0.0f;
        }
    }

    size_t UIInput::indexAt(float x, float y) const
    {
        UIFont* font = this->activeFont();
        Rect content = this->contentRect();
        if (multiline)
        {
            if (font == nullptr)
            {
                return 0;
            }
            std::vector<TextLine> lines = splitLines(text);
            float lineH = this->lineBox(font);
            float localY = y - content.y + mScrollY;
            int line = 0;
            if (lineH > 0.0f && localY >= lineH)
            {
                line = (int)floorf(localY / lineH);
            }
            if (line < 0)
            {
                line = 0;
            }
            if (line >= (int)lines.size())
            {
                line = (int)lines.size() - 1;
            }
            float localX = x - content.x + mScroll;
            const TextLine& item = lines[(size_t)line];
            return this->indexOnLine(text, item.begin, item.end, localX);
        }
        if (font == nullptr || text.isEmpty())
        {
            return 0;
        }
        float scale = this->textScale(font);
        float local = x - content.x + mScroll;
        if (local <= 0.0f)
        {
            return 0;
        }
        float width = 0.0f;
        size_t i = 0;
        while (i < text.length())
        {
            size_t before = i;
            uint32_t codepoint = 0;
            if (!UIFont::nextUtf8(text.cstr(), text.length(), i, codepoint))
            {
                break;
            }
            float advance = font->glyph(codepoint).advance * scale;
            if (local < width + advance * 0.5f)
            {
                return before;
            }
            width += advance;
        }
        return text.length();
    }

    void UIInput::render(UIShape& shape)
    {
        if (!visible)
        {
            return;
        }
        this->clampCaret();
        this->ensureCaretVisible();
        Color bg = hovered ? hoverColor : background;
        shape.addQuad(rect, UIFont::solidUV(), bg);
        if (focused)
        {
            this->drawBorder(shape);
        }
        Rect content = this->contentRect();
        UIFont* font = this->activeFont();
        if (font != nullptr && content.width > 0.0f && content.height > 0.0f)
        {
            if (!text.isEmpty())
            {
                this->drawSelection(shape, content);
                UIText* t = this->label();
                Color color = t != nullptr ? t->color : caretColor;
                this->drawGlyphRun(shape, text, color, content, mScroll);
            }
            else if (!placeholder.isEmpty())
            {
                this->drawGlyphRun(shape, placeholder, placeholderColor, content, 0.0f);
            }
            this->drawCaret(shape, content);
        }
        UIWidget::render(shape);
    }

    void UIInput::drawBorder(UIShape& shape) const
    {
        float t = borderThickness;
        if (t <= 0.0f || rect.width <= 0.0f || rect.height <= 0.0f)
        {
            return;
        }
        Rect uv = UIFont::solidUV();
        shape.addQuad(Rect(rect.x, rect.y, rect.width, t), uv, borderColor);
        shape.addQuad(Rect(rect.x, rect.y + rect.height - t, rect.width, t), uv, borderColor);
        shape.addQuad(Rect(rect.x, rect.y, t, rect.height), uv, borderColor);
        shape.addQuad(Rect(rect.x + rect.width - t, rect.y, t, rect.height), uv, borderColor);
    }

    void UIInput::drawSelection(UIShape& shape, const Rect& content) const
    {
        if (!this->hasSelection())
        {
            return;
        }
        size_t a = Math::min_s(mAnchor, mCaret);
        size_t b = Math::max_s(mAnchor, mCaret);
        if (multiline)
        {
            UIFont* font = this->activeFont();
            if (font == nullptr)
            {
                return;
            }
            std::vector<TextLine> lines = splitLines(text);
            float lineH = this->lineBox(font);
            float right = content.x + content.width;
            float bottom = content.y + content.height;
            for (size_t i = 0; i < lines.size(); ++i)
            {
                size_t begin = lines[i].begin;
                size_t end = lines[i].end;
                if (b <= begin || a > end)
                {
                    continue;
                }
                size_t sel0 = a > begin ? a : begin;
                if (sel0 > end)
                {
                    continue;
                }
                bool throughBreak = i + 1 < lines.size() && b > end;
                size_t sel1 = b < end ? b : end;
                float x0 = content.x + this->advanceBetween(text, begin, sel0) - mScroll;
                float x1 = throughBreak ? right : content.x + this->advanceBetween(text, begin, sel1) - mScroll;
                if (x0 < content.x)
                {
                    x0 = content.x;
                }
                if (x1 > right)
                {
                    x1 = right;
                }
                if (x1 <= x0)
                {
                    continue;
                }
                float y0 = content.y + (float)i * lineH - mScrollY;
                float h = lineH;
                float uvPos = 0.0f;
                float uvSize = 1.0f;
                if (!clipSpan(y0, h, uvPos, uvSize, content.y, bottom))
                {
                    continue;
                }
                shape.addQuad(Rect(snap(x0), snap(y0), snap(x1) - snap(x0), h), UIFont::solidUV(), selectionColor);
            }
            return;
        }
        float x0 = content.x + this->offsetOf(a) - mScroll;
        float x1 = content.x + this->offsetOf(b) - mScroll;
        if (x0 < content.x)
        {
            x0 = content.x;
        }
        if (x1 > content.x + content.width)
        {
            x1 = content.x + content.width;
        }
        if (x1 <= x0)
        {
            return;
        }
        shape.addQuad(Rect(snap(x0), snap(content.y), snap(x1) - snap(x0), content.height), UIFont::solidUV(), selectionColor);
    }

    void UIInput::drawGlyphRun(UIShape& shape, const String& value, const Color& color, const Rect& content, float scroll) const
    {
        UIFont* font = this->activeFont();
        if (font == nullptr)
        {
            return;
        }
        float scale = this->textScale(font);
        if (!multiline)
        {
            float tight = (font->ascender() - font->descender()) * scale;
            float textTop = snap(content.y + (content.height - tight) * 0.5f);
            float baseline = textTop + font->ascender() * scale;
            this->paintRange(shape, value, 0, value.length(), color, content, baseline, scroll);
            return;
        }
        std::vector<TextLine> lines = splitLines(value);
        float lineH = this->lineBox(font);
        float bottom = content.y + content.height;
        for (size_t i = 0; i < lines.size(); ++i)
        {
            float top = content.y - mScrollY + (float)i * lineH;
            if (top + lineH <= content.y || top >= bottom)
            {
                continue;
            }
            float baseline = top + font->ascender() * scale;
            this->paintRange(shape, value, lines[i].begin, lines[i].end, color, content, baseline, scroll);
        }
    }

    void UIInput::paintRange(UIShape& shape, const String& value, size_t begin, size_t end, const Color& color, const Rect& content, float baseline, float scrollX) const
    {
        UIFont* font = this->activeFont();
        if (font == nullptr || begin >= end)
        {
            return;
        }
        if (end > value.length())
        {
            end = value.length();
        }
        float scale = this->textScale(font);
        float cursorX = snap(content.x - scrollX);
        float right = content.x + content.width;
        size_t index = begin;
        while (index < end)
        {
            uint32_t codepoint = 0;
            if (!UIFont::nextUtf8(value.cstr(), value.length(), index, codepoint) || index > end)
            {
                continue;
            }
            const UIFontGlyph& g = font->glyph(codepoint);
            float advance = g.advance * scale;
            if (g.uv.width > 0.0f && g.uv.height > 0.0f)
            {
                float destX = cursorX + g.bearingX * scale;
                float destY = baseline - g.bearingY * scale;
                float destW = g.width * scale;
                float destH = g.height * scale;
                float u = g.uv.x;
                float v = g.uv.y;
                float uw = g.uv.width;
                float uh = g.uv.height;
                bool visibleX = clipSpan(destX, destW, u, uw, content.x, right);
                bool visibleY = clipSpan(destY, destH, v, uh, content.y, content.y + content.height);
                if (visibleX && visibleY)
                {
                    shape.addQuad(Rect(snap(destX), snap(destY), destW, destH), Rect(u, v, uw, uh), color);
                }
            }
            cursorX += advance;
        }
    }

    void UIInput::drawCaret(UIShape& shape, const Rect& content) const
    {
        if (!focused)
        {
            return;
        }
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - mCaretBlinkAnchor).count();
        constexpr int64_t kCaretBlinkMs = 530;
        if (((elapsed / kCaretBlinkMs) % 2) != 0)
        {
            return;
        }
        if (multiline)
        {
            UIFont* font = this->activeFont();
            if (font == nullptr)
            {
                return;
            }
            std::vector<TextLine> lines = splitLines(text);
            size_t line = lineOf(lines, mCaret);
            float lineH = this->lineBox(font);
            float x = snap(content.x + this->offsetOf(mCaret) - mScroll);
            if (x < content.x || x >= content.x + content.width)
            {
                return;
            }
            float y = content.y + (float)line * lineH - mScrollY;
            float h = lineH;
            float uvPos = 0.0f;
            float uvSize = 1.0f;
            if (!clipSpan(y, h, uvPos, uvSize, content.y, content.y + content.height))
            {
                return;
            }
            shape.addQuad(Rect(x, snap(y), 1.0f, h), UIFont::solidUV(), caretColor);
            return;
        }
        float x = snap(content.x + this->offsetOf(mCaret) - mScroll);
        if (x < content.x || x >= content.x + content.width)
        {
            return;
        }
        shape.addQuad(Rect(x, content.y, 1.0f, content.height), UIFont::solidUV(), caretColor);
    }

    void UIInput::triggerPointerPress()
    {
        UIWidget::triggerPointerPress();
        mPointerSelecting = true;
        mCaretPlaced = false;
    }

    void UIInput::triggerPointerDrag(float x, float y, int button)
    {
        if (button != 0 || !mPointerSelecting)
        {
            return;
        }
        size_t index = this->indexAt(x, y);
        mPreferredX = -1.0f;
        if (!mCaretPlaced)
        {
            mAnchor = index;
            mCaretPlaced = true;
        }
        mCaret = index;
        this->ensureCaretVisible();
        this->resetCaretBlink();
    }

    void UIInput::triggerPointerRelease()
    {
        mPointerSelecting = false;
        mCaretPlaced = false;
        UIWidget::triggerPointerRelease();
    }

    void UIInput::triggerFocus()
    {
        UIWidget::triggerFocus();
        this->resetCaretBlink();
    }

    void UIInput::triggerBlur()
    {
        mPointerSelecting = false;
        mCaretPlaced = false;
        UIWidget::triggerBlur();
    }

    void UIInput::triggerChar(uint32_t codepoint)
    {
        if (codepoint < 32 || codepoint == 127)
        {
            return;
        }
        this->insertText(UIFont::encodeUtf8(codepoint));
    }

    void UIInput::triggerKey(UIKey key, const UIKeyMods& mods)
    {
        switch (key)
        {
        case UIKey::Backspace:
            this->deleteCaret(true);
            break;
        case UIKey::Delete:
            this->deleteCaret(false);
            break;
        case UIKey::Left:
            this->moveCaretBy(-1, mods.shift);
            break;
        case UIKey::Right:
            this->moveCaretBy(1, mods.shift);
            break;
        case UIKey::Up:
            this->moveCaretVertically(-1, mods.shift);
            break;
        case UIKey::Down:
            this->moveCaretVertically(1, mods.shift);
            break;
        case UIKey::Home:
            if (multiline && !mods.ctrl)
            {
                std::vector<TextLine> lines = splitLines(text);
                this->moveCaret(lines[lineOf(lines, mCaret)].begin, mods.shift);
            }
            else
            {
                this->moveCaret(0, mods.shift);
            }
            break;
        case UIKey::End:
            if (multiline && !mods.ctrl)
            {
                std::vector<TextLine> lines = splitLines(text);
                this->moveCaret(lines[lineOf(lines, mCaret)].end, mods.shift);
            }
            else
            {
                this->moveCaret(text.length(), mods.shift);
            }
            break;
        case UIKey::Enter:
            if (multiline && !mods.ctrl)
            {
                this->insertText("\n");
            }
            else if (onSubmit)
            {
                onSubmit(text);
            }
            break;
        case UIKey::A:
            if (mods.ctrl)
            {
                mAnchor = 0;
                mCaret = text.length();
                mPreferredX = -1.0f;
                this->ensureCaretVisible();
                this->resetCaretBlink();
            }
            break;
        case UIKey::C:
            if (mods.ctrl && writeClipboard && this->hasSelection())
            {
                writeClipboard(this->selectedText());
            }
            break;
        case UIKey::X:
            if (mods.ctrl && writeClipboard && this->hasSelection())
            {
                writeClipboard(this->selectedText());
                this->insertText(String());
            }
            break;
        case UIKey::V:
            if (mods.ctrl && readClipboard)
            {
                this->insertText(readClipboard());
            }
            break;
        case UIKey::Escape:
            break;
        }
    }

    void UIInput::resetPointerState()
    {
        mPointerSelecting = false;
        mCaretPlaced = false;
        UIWidget::resetPointerState();
    }
}
