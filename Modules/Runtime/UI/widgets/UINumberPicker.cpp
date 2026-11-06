#include "UINumberPicker.h"
#include "../UIFont.h"
#include "../UIStroke.h"
#include <cmath>

namespace eokas
{
    namespace
    {
        constexpr float kRow = 36.0f;
        constexpr float kButton = 22.0f;
        constexpr float kWidth = 52.0f;
        constexpr int kVisible = 5;
        constexpr float kSlop = 4.0f;
        constexpr float kFriction = 6.0f;
        constexpr float kCoastMin = 0.45f;
        constexpr float kSnapSeconds = 0.14f;

        float snapf(float v)
        {
            return floorf(v + 0.5f);
        }

        float clampf(float v, float lo, float hi)
        {
            if (v < lo) return lo;
            if (v > hi) return hi;
            return v;
        }

        float easeOut(float t)
        {
            t = clampf(t, 0.0f, 1.0f);
            return 1.0f - (1.0f - t) * (1.0f - t);
        }

        Rect aabbScreen(const Vector2 corners[4])
        {
            float x = corners[0].x;
            float y = corners[0].y;
            float r = corners[0].x;
            float b = corners[0].y;
            for (int i = 1; i < 4; ++i)
            {
                if (corners[i].x < x) x = corners[i].x;
                if (corners[i].y < y) y = corners[i].y;
                if (corners[i].x > r) r = corners[i].x;
                if (corners[i].y > b) b = corners[i].y;
            }
            return Rect(x, y, r - x, b - y);
        }
    }

    UINumberPicker::UINumberPicker()
    {
        focusable = true;
        pickable = true;
        UIWidget::color.a = 0.0f;
        mScroll = (float)value;
        mLastTick = std::chrono::steady_clock::now();
        mLastMove = mLastTick;
    }

    Vector2 UINumberPicker::preferredSize(float scale, UIDirection direction)
    {
        if (!(scale > 0.0f)) scale = 1.0f;
        float cross = snapf(kWidth * scale);
        float main = snapf(kRow * (float)kVisible * scale);
        if (direction == UIDirection::Horizontal)
        {
            return Vector2(main, cross);
        }
        return Vector2(cross, main);
    }

    void UINumberPicker::setRange(int minV, int maxV)
    {
        if (minV > maxV)
        {
            int tmp = minV;
            minV = maxV;
            maxV = tmp;
        }
        minValue = minV;
        maxValue = maxV;
        this->setValue(value);
    }

    void UINumberPicker::setValue(int v)
    {
        int next = this->normalize(v);
        bool changed = next != value;
        value = next;
        if (mMotion != Motion::Drag && mMotion != Motion::Coast && mMotion != Motion::Snap)
        {
            mScroll = (float)value;
        }
        else if (!wrap)
        {
            mScroll = this->clampScroll(mScroll);
        }
        if (changed)
        {
            this->notifyChanged();
        }
    }

    void UINumberPicker::setFont(const String& path, float size)
    {
        fontPath = path;
        if (size > 0.0f) fontSize = size;
    }

    void UINumberPicker::step(int delta, bool commit)
    {
        this->applyDelta(delta, commit);
    }

    bool UINumberPicker::horizontal() const
    {
        return direction == UIDirection::Horizontal;
    }

    float UINumberPicker::scaleOf() const
    {
        return uiScale > 0.0f ? uiScale : 1.0f;
    }

    float UINumberPicker::itemExtent() const
    {
        float row = kRow * this->scaleOf();
        return row > 1.0f ? row : 1.0f;
    }

    float UINumberPicker::buttonExtent() const
    {
        float h = kButton * this->scaleOf();
        float half = this->axisSize() * 0.28f;
        if (h > half) h = half;
        return h > 8.0f ? h : 8.0f;
    }

    float UINumberPicker::axisPos(const Vector2& local) const
    {
        return this->horizontal() ? local.x : local.y;
    }

    float UINumberPicker::axisSize() const
    {
        return this->horizontal() ? shape.size.x : shape.size.y;
    }

    int UINumberPicker::count() const
    {
        int span = maxValue - minValue + 1;
        return span > 0 ? span : 1;
    }

    int UINumberPicker::normalize(int v) const
    {
        if (maxValue < minValue)
        {
            return minValue;
        }
        if (wrap)
        {
            int span = this->count();
            int r = (v - minValue) % span;
            if (r < 0) r += span;
            return minValue + r;
        }
        if (v < minValue) return minValue;
        if (v > maxValue) return maxValue;
        return v;
    }

    float UINumberPicker::wrapScroll(float scroll) const
    {
        if (!wrap) return scroll;
        float span = (float)this->count();
        float origin = (float)minValue;
        float shifted = scroll - origin;
        shifted = fmodf(shifted, span);
        if (shifted < 0.0f) shifted += span;
        return origin + shifted;
    }

    float UINumberPicker::clampScroll(float scroll) const
    {
        if (wrap) return this->wrapScroll(scroll);
        return clampf(scroll, (float)minValue, (float)maxValue);
    }

    int UINumberPicker::nearestValue(float scroll) const
    {
        float target = this->snapTarget(scroll);
        return this->normalize((int)floorf(target + 0.5f));
    }

    float UINumberPicker::snapTarget(float scroll) const
    {
        if (wrap)
        {
            return floorf(scroll + 0.5f);
        }
        return clampf(floorf(scroll + 0.5f), (float)minValue, (float)maxValue);
    }

    String UINumberPicker::labelOf(int v) const
    {
        if (formatter) return formatter(v);
        return String::format("%d", v);
    }

    UINumberPicker::Part UINumberPicker::hitTest(float x, float y) const
    {
        if (x < 0.0f || y < 0.0f || x > shape.size.x || y > shape.size.y)
        {
            return Part::None;
        }
        float btn = this->buttonExtent();
        float pos = this->axisPos(Vector2(x, y));
        if (pos < btn) return Part::Prev;
        if (pos > this->axisSize() - btn) return Part::Next;
        return Part::Wheel;
    }

    Rect UINumberPicker::wheelClipScreen() const
    {
        float btn = this->buttonExtent();
        Rect local;
        if (this->horizontal())
        {
            local = Rect(btn, 0.0f, shape.size.x - btn * 2.0f, shape.size.y);
        }
        else
        {
            local = Rect(0.0f, btn, shape.size.x, shape.size.y - btn * 2.0f);
        }
        if (local.size.x < 1.0f || local.size.y < 1.0f)
        {
            return this->screenBounds();
        }
        Vector2 corners[4] = {
            this->localToScreen(Vector2(local.origin.x, local.origin.y)),
            this->localToScreen(Vector2(local.origin.x + local.size.x, local.origin.y)),
            this->localToScreen(Vector2(local.origin.x + local.size.x, local.origin.y + local.size.y)),
            this->localToScreen(Vector2(local.origin.x, local.origin.y + local.size.y))
        };
        return aabbScreen(corners);
    }

    void UINumberPicker::syncValue(bool notify)
    {
        int next = this->nearestValue(mScroll);
        if (next == value)
        {
            return;
        }
        value = next;
        if (notify)
        {
            mDirty = true;
            this->notifyChanged();
        }
    }

    void UINumberPicker::notifyChanged()
    {
        if (onValueChanged)
        {
            onValueChanged(value);
        }
    }

    void UINumberPicker::notifyCommitted()
    {
        mDirty = false;
        if (onValueCommitted)
        {
            onValueCommitted(value);
        }
    }

    void UINumberPicker::stopMotion()
    {
        mMotion = Motion::Idle;
        mVelocity = 0.0f;
        mScroll = (float)value;
    }

    void UINumberPicker::beginSnap(float target)
    {
        mSnapFrom = mScroll;
        mSnapTo = target;
        if (fabsf(mSnapTo - mSnapFrom) < 0.001f)
        {
            mScroll = this->clampScroll(mSnapTo);
            this->syncValue(true);
            mMotion = Motion::Idle;
            mVelocity = 0.0f;
            if (mDirty) this->notifyCommitted();
            return;
        }
        mMotion = Motion::Snap;
        mVelocity = 0.0f;
        mSnapStart = std::chrono::steady_clock::now();
    }

    void UINumberPicker::finishDrag()
    {
        if (mMotion != Motion::Drag)
        {
            return;
        }
        mScroll = this->clampScroll(mScroll);
        this->syncValue(true);
        bool coast = fabsf(mVelocity) > kCoastMin;
        if (coast && !wrap)
        {
            if ((mVelocity < 0.0f && mScroll <= (float)minValue) || (mVelocity > 0.0f && mScroll >= (float)maxValue))
            {
                coast = false;
            }
        }
        if (coast)
        {
            mMotion = Motion::Coast;
            mLastTick = std::chrono::steady_clock::now();
            return;
        }
        this->beginSnap(this->snapTarget(mScroll));
    }

    void UINumberPicker::applyDelta(int delta, bool commit)
    {
        if (delta == 0)
        {
            if (commit && mDirty) this->notifyCommitted();
            return;
        }
        this->stopMotion();
        int next = this->normalize(value + delta);
        if (wrap)
        {
            mSnapFrom = mScroll;
            mSnapTo = mScroll + (float)delta;
            bool changed = next != value;
            value = next;
            if (changed)
            {
                this->notifyChanged();
            }
            if (commit)
            {
                mDirty = false;
                this->notifyCommitted();
                this->beginSnap(mSnapTo);
            }
            else
            {
                mDirty = changed;
                mScroll = mSnapTo;
            }
            return;
        }
        this->setValue(next);
        mScroll = (float)value;
        if (commit)
        {
            mDirty = false;
            this->notifyCommitted();
        }
        else
        {
            mDirty = true;
        }
    }

    void UINumberPicker::tickMotion()
    {
        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - mLastTick).count();
        mLastTick = now;
        if (dt < 0.0f) dt = 0.0f;
        if (dt > 0.05f) dt = 0.05f;

        if (mMotion == Motion::Coast)
        {
            mScroll += mVelocity * dt;
            mScroll = this->clampScroll(mScroll);
            mVelocity *= expf(-kFriction * dt);
            this->syncValue(true);
            bool stop = fabsf(mVelocity) < kCoastMin;
            if (!wrap)
            {
                if ((mVelocity < 0.0f && mScroll <= (float)minValue) || (mVelocity > 0.0f && mScroll >= (float)maxValue))
                {
                    stop = true;
                }
            }
            if (stop)
            {
                this->beginSnap(this->snapTarget(mScroll));
            }
            return;
        }

        if (mMotion != Motion::Snap)
        {
            return;
        }
        float t = std::chrono::duration<float>(now - mSnapStart).count() / kSnapSeconds;
        if (t >= 1.0f)
        {
            mScroll = this->clampScroll(mSnapTo);
            this->syncValue(true);
            mMotion = Motion::Idle;
            mVelocity = 0.0f;
            if (mDirty) this->notifyCommitted();
            return;
        }
        mScroll = mSnapFrom + (mSnapTo - mSnapFrom) * easeOut(t);
    }

    void UINumberPicker::layout()
    {
        if (!visible)
        {
            return;
        }
        if (mMotion != Motion::Drag)
        {
            this->tickMotion();
        }
        UIWidget::layout();
    }

    void UINumberPicker::drawChevron(UIPrimitive& primitive, float cx, float cy, float size, Part part, const Color& color) const
    {
        Rect uv = UIFont::solidUV();
        float half = size * 0.5f;
        bool prev = part == Part::Prev;
        if (this->horizontal())
        {
            if (prev)
            {
                primitive.addQuad(
                    this->localToScreen(Vector2(cx + size * 0.25f, cy - half)),
                    this->localToScreen(Vector2(cx + size * 0.25f, cy + half)),
                    this->localToScreen(Vector2(cx - size * 0.35f, cy)),
                    this->localToScreen(Vector2(cx - size * 0.35f, cy)),
                    uv,
                    color);
                return;
            }
            primitive.addQuad(
                this->localToScreen(Vector2(cx - size * 0.25f, cy - half)),
                this->localToScreen(Vector2(cx - size * 0.25f, cy + half)),
                this->localToScreen(Vector2(cx + size * 0.35f, cy)),
                this->localToScreen(Vector2(cx + size * 0.35f, cy)),
                uv,
                color);
            return;
        }
        if (prev)
        {
            primitive.addQuad(
                this->localToScreen(Vector2(cx - half, cy + size * 0.25f)),
                this->localToScreen(Vector2(cx + half, cy + size * 0.25f)),
                this->localToScreen(Vector2(cx, cy - size * 0.35f)),
                this->localToScreen(Vector2(cx, cy - size * 0.35f)),
                uv,
                color);
            return;
        }
        primitive.addQuad(
            this->localToScreen(Vector2(cx - half, cy - size * 0.25f)),
            this->localToScreen(Vector2(cx + half, cy - size * 0.25f)),
            this->localToScreen(Vector2(cx, cy + size * 0.35f)),
            this->localToScreen(Vector2(cx, cy + size * 0.35f)),
            uv,
            color);
    }

    void UINumberPicker::drawLabel(UIPrimitive& primitive, const String& text, float cx, float cy, const Color& color) const
    {
        UIFont* font = UIFont::find(fontPath);
        if (font == nullptr || !font->isOpen() || text.isEmpty())
        {
            return;
        }
        Vector2 measured = font->measure(text, fontSize);
        float scale = 1.0f;
        float ascender = 0.0f;
        float descender = 0.0f;
        font->drawMetrics(fontSize, scale, ascender, descender);
        float left = snapf(cx - measured.x * 0.5f);
        float top = snapf(cy - measured.y * 0.5f);
        float baseline = floorf(top + ascender + 0.5f);
        float cursorX = left;
        size_t index = 0;
        while (index < text.length())
        {
            uint32_t codepoint = 0;
            if (!UIFont::nextUtf8(text.cstr(), text.length(), index, codepoint))
            {
                continue;
            }
            const UIFontGlyph& g = font->glyphSized(codepoint, fontSize);
            if (g.uv.size.x > 0.0f && g.uv.size.y > 0.0f)
            {
                float destX = floorf(cursorX + g.bearingX * scale + 0.5f);
                float destY = floorf(baseline - g.bearingY * scale + 0.5f);
                float destW = (float)(int)(g.width * scale + 0.5f);
                float destH = (float)(int)(g.height * scale + 0.5f);
                primitive.addQuad(this->matrixLocalToScreen(), Rect(destX, destY, destW, destH), g.uv, color);
            }
            cursorX += g.advance * scale;
        }
    }

    void UINumberPicker::render(UIPrimitive& primitive)
    {
        if (!visible)
        {
            return;
        }
        Matrix3 localToScreen = this->matrixLocalToScreen();
        Rect uv = UIFont::solidUV();
        if (background.a > 0.0f)
        {
            primitive.addQuad(localToScreen, Rect(0.0f, 0.0f, shape.size.x, shape.size.y), uv, background);
        }
        if (field.a > 0.0f)
        {
            primitive.addQuad(localToScreen, Rect(0.0f, 0.0f, shape.size.x, shape.size.y), uv, field);
        }

        float item = this->itemExtent();
        float center = this->axisSize() * 0.5f;
        Color band = accent;
        band.a = 0.22f;
        Color edge = accent;
        edge.a = 0.55f;
        float lineH = snapf(this->scaleOf());
        if (lineH < 1.0f) lineH = 1.0f;
        if (this->horizontal())
        {
            float bandX = snapf(center - item * 0.5f);
            primitive.addQuad(localToScreen, Rect(bandX, 0.0f, item, shape.size.y), uv, band);
            primitive.addQuad(localToScreen, Rect(bandX, 0.0f, lineH, shape.size.y), uv, edge);
            primitive.addQuad(localToScreen, Rect(bandX + item - lineH, 0.0f, lineH, shape.size.y), uv, edge);
        }
        else
        {
            float bandY = snapf(center - item * 0.5f);
            primitive.addQuad(localToScreen, Rect(0.0f, bandY, shape.size.x, item), uv, band);
            primitive.addQuad(localToScreen, Rect(0.0f, bandY, shape.size.x, lineH), uv, edge);
            primitive.addQuad(localToScreen, Rect(0.0f, bandY + item - lineH, shape.size.x, lineH), uv, edge);
        }

        primitive.pushClip(this->wheelClipScreen());
        float scroll = wrap ? mScroll : this->clampScroll(mScroll);
        int extra = kVisible;
        int nearest = (int)floorf(scroll + 0.5f);
        for (int i = -extra; i <= extra; ++i)
        {
            int raw = nearest + i;
            if (!wrap && (raw < minValue || raw > maxValue))
            {
                continue;
            }
            int shown = this->normalize(raw);
            float pos = center + ((float)raw - scroll) * item;
            if (pos < -item || pos > this->axisSize() + item)
            {
                continue;
            }
            float dist = fabsf((float)raw - scroll);
            float alpha = 1.0f - dist / 2.35f;
            if (alpha < 0.12f) alpha = 0.12f;
            if (alpha > 1.0f) alpha = 1.0f;
            Color color = dist < 0.5f ? ink : muted;
            color.a *= alpha;
            if (this->horizontal())
            {
                this->drawLabel(primitive, this->labelOf(shown), pos, shape.size.y * 0.5f, color);
            }
            else
            {
                this->drawLabel(primitive, this->labelOf(shown), shape.size.x * 0.5f, pos, color);
            }
        }
        primitive.popClip();

        float btn = this->buttonExtent();
        float chev = snapf(6.0f * this->scaleOf());
        if (chev < 4.0f) chev = 4.0f;
        auto buttonFill = [&](Part part)
        {
            if (!pickable) return muted;
            if (mPressPart == part && isPressed()) return buttonPress;
            if (mHover == part) return buttonHover;
            return Color(0.0f, 0.0f, 0.0f, 0.0f);
        };
        auto buttonInk = [&](Part part)
        {
            if (!pickable) return muted;
            if (mPressPart == part && isPressed()) return ink;
            if (mHover == part) return ink;
            return muted;
        };
        auto paintButton = [&](Part part, const Rect& rect, float cx, float cy)
        {
            Color fill = buttonFill(part);
            if (fill.a > 0.0f)
            {
                primitive.addQuad(localToScreen, rect, uv, fill);
            }
            this->drawChevron(primitive, snapf(cx), snapf(cy), chev, part, buttonInk(part));
        };
        if (this->horizontal())
        {
            paintButton(Part::Prev, Rect(0.0f, 0.0f, btn, shape.size.y), btn * 0.5f, shape.size.y * 0.5f);
            paintButton(Part::Next, Rect(shape.size.x - btn, 0.0f, btn, shape.size.y), shape.size.x - btn * 0.5f, shape.size.y * 0.5f);
        }
        else
        {
            paintButton(Part::Prev, Rect(0.0f, 0.0f, shape.size.x, btn), shape.size.x * 0.5f, btn * 0.5f);
            paintButton(Part::Next, Rect(0.0f, shape.size.y - btn, shape.size.x, btn), shape.size.x * 0.5f, shape.size.y - btn * 0.5f);
        }

        if (isFocused())
        {
            UIStrokeStyle border = line;
            border.color = accent;
            border.thickness = this->scaleOf();
            UIStroke::border(primitive, localToScreen, Rect(0.0f, 0.0f, shape.size.x, shape.size.y), border);
        }
        UIWidget::render(primitive);
    }

    bool UINumberPicker::handlePointerPress(const UIPointerEvent& event)
    {
        UIWidget::handlePointerPress(event);
        Vector2 local = this->screenToLocal(event.position);
        mPressPart = this->hitTest(local.x, local.y);
        mPressLocal = this->axisPos(local);
        mLastLocal = mPressLocal;
        mPressScroll = mScroll;
        mMoved = false;
        mVelocity = 0.0f;
        mLastMove = std::chrono::steady_clock::now();
        mLastTick = mLastMove;
        if (mPressPart == Part::Wheel)
        {
            this->stopMotion();
            mScroll = mPressScroll;
        }
        mHover = mPressPart;
        return true;
    }

    bool UINumberPicker::handlePointerMove(const UIPointerEvent& event)
    {
        Vector2 local = this->screenToLocal(event.position);
        bool handled = UIWidget::handlePointerMove(event);
        if (!isPressed())
        {
            mHover = this->hitTest(local.x, local.y);
            return handled;
        }
        mHover = mPressPart;
        float pos = this->axisPos(local);
        float d = pos - mLastLocal;
        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - mLastMove).count();
        if (dt < 0.0001f) dt = 0.0001f;
        mLastMove = now;
        mLastLocal = pos;
        if (mPressPart != Part::Wheel)
        {
            return true;
        }
        if (fabsf(pos - mPressLocal) > kSlop * this->scaleOf())
        {
            mMoved = true;
        }
        if (mMoved)
        {
            mMotion = Motion::Drag;
            float item = this->itemExtent();
            mScroll = this->clampScroll(mPressScroll - (pos - mPressLocal) / item);
            mVelocity = -(d / item) / dt;
            this->syncValue(true);
        }
        return true;
    }

    bool UINumberPicker::handlePointerRelease(const UIPointerEvent& event)
    {
        (void)event;
        if (mMotion == Motion::Drag)
        {
            this->finishDrag();
        }
        bool handled = UIWidget::handlePointerRelease(event);
        mPressPart = Part::None;
        return handled;
    }

    bool UINumberPicker::handleClick(float screenX, float screenY)
    {
        if (mMoved)
        {
            return true;
        }
        Vector2 local = this->screenToLocal(Vector2(screenX, screenY));
        Part part = this->hitTest(local.x, local.y);
        if (part == Part::Prev)
        {
            this->applyDelta(-1, true);
            return true;
        }
        if (part == Part::Next)
        {
            this->applyDelta(1, true);
            return true;
        }
        if (part == Part::Wheel)
        {
            float rel = (this->axisPos(local) - this->axisSize() * 0.5f) / this->itemExtent();
            int delta = (int)floorf(rel + 0.5f);
            this->applyDelta(delta, true);
            return true;
        }
        return UIWidget::handleClick(screenX, screenY);
    }

    bool UINumberPicker::handleWheel(float screenX, float screenY, float deltaX, float deltaY)
    {
        Vector2 local = this->screenToLocal(Vector2(screenX, screenY));
        if (!this->contains(local))
        {
            return false;
        }
        float notches = 0.0f;
        if (this->horizontal())
        {
            notches = deltaX / 48.0f;
            if (fabsf(notches) < 0.01f)
            {
                notches = -deltaY / 48.0f;
            }
        }
        else
        {
            notches = -deltaY / 48.0f;
            if (fabsf(notches) < 0.01f)
            {
                notches = deltaX / 48.0f;
            }
        }
        if (fabsf(notches) < 0.01f)
        {
            return false;
        }
        int delta = (int)floorf(notches + (notches > 0.0f ? 0.5f : -0.5f));
        if (delta == 0) delta = notches > 0.0f ? 1 : -1;
        this->applyDelta(delta, true);
        return true;
    }

    bool UINumberPicker::handleKeyPress(const UIKeyEvent& event)
    {
        if (!pickable)
        {
            return UIWidget::handleKeyPress(event);
        }
        if (event.key == UIKeyCode::Up || event.key == UIKeyCode::Left)
        {
            this->applyDelta(-1, true);
            return true;
        }
        if (event.key == UIKeyCode::Down || event.key == UIKeyCode::Right)
        {
            this->applyDelta(1, true);
            return true;
        }
        if (event.key == UIKeyCode::PageUp)
        {
            this->applyDelta(-5, true);
            return true;
        }
        if (event.key == UIKeyCode::PageDown)
        {
            this->applyDelta(5, true);
            return true;
        }
        if (event.key == UIKeyCode::Home)
        {
            this->stopMotion();
            this->setValue(minValue);
            this->notifyCommitted();
            return true;
        }
        if (event.key == UIKeyCode::End)
        {
            this->stopMotion();
            this->setValue(maxValue);
            this->notifyCommitted();
            return true;
        }
        return UIWidget::handleKeyPress(event);
    }

    void UINumberPicker::handleFocusLoss()
    {
        if (mMotion != Motion::Idle)
        {
            mScroll = this->clampScroll(this->snapTarget(mScroll));
            this->syncValue(true);
            this->stopMotion();
            if (mDirty) this->notifyCommitted();
        }
        UIWidget::handleFocusLoss();
    }

    void UINumberPicker::endActiveDrag()
    {
        if (mMotion == Motion::Drag)
        {
            this->finishDrag();
        }
    }
}
