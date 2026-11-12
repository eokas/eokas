#include "./SchemaPage.h"
#include "./Theme.h"
#include "./UiKit.h"

#include "UI/Main.h"
#include "UI/UIFont.h"

namespace eokas::RoseAxion {

    void RoseSchemaPage::bind(TableDocument& doc, const Host& host)
    {
        mDoc = &doc;
        mHost = host;
    }

    void RoseSchemaPage::setOverlay(UIWidget* overlay)
    {
        if (mOutliner)
        {
            mOutliner->setOverlay(overlay);
        }
        if (mSchemaEditor)
        {
            mSchemaEditor->setOverlay(overlay);
        }
        mOverlay = overlay;
    }

    const String& RoseSchemaPage::font() const
    {
        return mHost.font;
    }

    float RoseSchemaPage::scale() const
    {
        return hostScale(mHost.scale);
    }

    RoseSchemaEditor::Host RoseSchemaPage::editorHost() const
    {
        RoseSchemaEditor::Host host;
        host.requestRebuild = mHost.requestRebuild;
        host.writeClipboard = mHost.writeClipboard;
        host.readClipboard = mHost.readClipboard;
        host.font = mHost.font;
        host.scale = mHost.scale;
        return host;
    }

    RoseSchemaOutliner::Host RoseSchemaPage::outlinerHost() const
    {
        RoseSchemaOutliner::Host host;
        host.requestRebuild = mHost.requestRebuild;
        host.confirmDeleteSchema = mHost.confirmDeleteSchema;
        host.font = mHost.font;
        host.scale = mHost.scale;
        return host;
    }

    void RoseSchemaPage::reset()
    {
        mResizePane = false;
        this->setOverlay(nullptr);
        if (mSchemaEditor)
        {
            mSchemaEditor->reset();
        }
        mSplit = nullptr;
        mSplitHandle = nullptr;
        mHint = nullptr;
        this->detachChildren();
    }

    void RoseSchemaPage::resetOutliner()
    {
        mKeepOutlinerY = 0.0f;
        mKeepEditorY = 0.0f;
        if (mOutliner)
        {
            mOutliner->setOverlay(nullptr);
            mOutliner->reset();
        }
        if (mSchemaEditor)
        {
            mSchemaEditor->setOverlay(nullptr);
            mSchemaEditor->reset();
        }
    }

    UIWidget* RoseSchemaPage::focusWidget() const
    {
        if (mOutliner != nullptr)
        {
            if (UIWidget* focus = mOutliner->focusWidget())
            {
                return focus;
            }
        }
        return mSchemaEditor != nullptr ? mSchemaEditor->focusWidget() : nullptr;
    }

    float RoseSchemaPage::outlinerWidth() const
    {
        const float s = this->scale();
        const float minW = 160.0f * s;
        float maxW = shape.size.x - 80.0f * s;
        if (maxW < minW)
        {
            maxW = minW;
        }
        float width = mOutlinerW * s;
        if (width > maxW)
        {
            width = maxW;
        }
        if (width < minW)
        {
            width = minW;
        }
        return width;
    }

    void RoseSchemaPage::paneRects(float& outlinerW, float& editorX, float& editorW) const
    {
        outlinerW = this->outlinerWidth();
        editorX = outlinerW + 1.0f;
        editorW = shape.size.x - editorX;
        if (editorW < 40.0f)
        {
            editorW = 40.0f;
        }
    }

    void RoseSchemaPage::layout()
    {
        if (!visible)
        {
            return;
        }
        float outlinerW = 0.0f;
        float editorX = 0.0f;
        float editorW = 0.0f;
        this->paneRects(outlinerW, editorX, editorW);
        const float height = shape.size.y;
        if (mOutliner != nullptr)
        {
            setBox(*mOutliner, Rect(0.0f, 0.0f, outlinerW, height));
        }
        if (mSplit != nullptr)
        {
            setBox(*mSplit, Rect(outlinerW, 0.0f, 1.0f, height));
        }
        this->placePaneHandle();
        if (mSchemaEditor != nullptr && mSchemaEditor->parent() == this)
        {
            setBox(*mSchemaEditor, Rect(editorX, 0.0f, editorW, height));
            mSchemaEditor->layout();
        }
        if (mHint != nullptr)
        {
            mHint->layout();
            setBox(*mHint, Rect(editorX + (editorW - mHint->shape.size.x) * 0.5f, (height - mHint->shape.size.y) * 0.5f, mHint->shape.size.x, mHint->shape.size.y));
        }
        if (mOutliner != nullptr)
        {
            mOutliner->layout();
        }
        UIWidget::layout();
    }

    bool RoseSchemaPage::handleEscape()
    {
        if (mOutliner != nullptr && mOutliner->handleEscape())
        {
            return true;
        }
        if (mResizePane)
        {
            this->cancelResize();
            return true;
        }
        return mSchemaEditor != nullptr && mSchemaEditor->handleEscape();
    }

    bool RoseSchemaPage::resizeCursorAt(float x, float y) const
    {
        if (mResizePane)
        {
            return true;
        }
        return this->hitResizePane(x, y);
    }

    bool RoseSchemaPage::hitResizePane(float screenX, float screenY) const
    {
        if (mSplitHandle != nullptr)
        {
            Vector2 local = mSplitHandle->screenToLocal(Vector2(screenX, screenY));
            return mSplitHandle->contains(local);
        }
        Vector2 local = this->screenToLocal(Vector2(screenX, screenY));
        const float grip = 6.0f * this->scale();
        float outlinerW = this->outlinerWidth();
        return local.y >= 0.0f && local.y <= shape.size.y
            && local.x >= outlinerW - grip * 0.5f && local.x <= outlinerW + grip * 0.5f;
    }

    void RoseSchemaPage::placePaneHandle()
    {
        if (mSplitHandle == nullptr)
        {
            return;
        }
        const float grip = 6.0f * this->scale();
        float outlinerW = this->outlinerWidth();
        setBox(*mSplitHandle, Rect(outlinerW - grip * 0.5f, 0.0f, grip, shape.size.y));
    }

    void RoseSchemaPage::applyPaneWidth(float pixelWidth)
    {
        const float s = this->scale();
        const float minW = 160.0f * s;
        float maxW = shape.size.x - 80.0f * s;
        if (maxW < minW)
        {
            maxW = minW;
        }
        if (pixelWidth < minW)
        {
            pixelWidth = minW;
        }
        if (pixelWidth > maxW)
        {
            pixelWidth = maxW;
        }
        mOutlinerW = pixelWidth / s;
        this->layout();
        if (mOutliner != nullptr)
        {
            mKeepOutlinerY = mOutliner->scrollY();
            mOutliner->setScroll(mKeepOutlinerY);
            mOutliner->rebuild();
            mOutliner->layout();
        }
        if (mSchemaEditor != nullptr && mSchemaEditor->parent() == this)
        {
            mKeepEditorY = mSchemaEditor->scrollY();
            mSchemaEditor->setScroll(mKeepEditorY);
            mSchemaEditor->rebuild();
            mSchemaEditor->layout();
        }
    }

    void RoseSchemaPage::commitPaneResize()
    {
        mResizePane = false;
    }

    void RoseSchemaPage::attachPaneHandle()
    {
        const float grip = 6.0f * this->scale();
        float outlinerW = this->outlinerWidth();
        auto handle = std::make_shared<UIWidget>();
        handle->overlay = true;
        handle->pickable = true;
        handle->color = Color(0.0f, 0.0f, 0.0f, 0.0f);
        handle->shape.pivot = Vector2(0.0f, 0.0f);
        setBox(handle, Rect(outlinerW - grip * 0.5f, 0.0f, grip, shape.size.y));
        handle->onPointerPress = [this](const UIPointerEvent& event)
        {
            if (event.pointerCode != UIPointerCode::Left)
            {
                return;
            }
            mResizePane = true;
            mResizeOriginPaneW = this->outlinerWidth();
        };
        handle->onDrag = [this](const Vector2&, const Vector2& delta)
        {
            if (!mResizePane)
            {
                return;
            }
            this->applyPaneWidth(mResizeOriginPaneW + delta.x);
        };
        handle->onPointerRelease = [this](const UIPointerEvent&)
        {
            this->commitPaneResize();
        };
        handle->onDrop = [this](const Vector2&, const Vector2&)
        {
            this->commitPaneResize();
        };
        this->attachChild(handle);
        mSplitHandle = handle.get();
    }

    void RoseSchemaPage::cancelResize()
    {
        if (mResizePane)
        {
            this->applyPaneWidth(mResizeOriginPaneW);
        }
        mResizePane = false;
    }

    void RoseSchemaPage::rebuild()
    {
        if (mOutliner)
        {
            mKeepOutlinerY = mOutliner->scrollY();
            mOutliner->setOverlay(nullptr);
        }
        if (mSchemaEditor)
        {
            mKeepEditorY = mSchemaEditor->scrollY();
            mSchemaEditor->setOverlay(nullptr);
        }
        this->detachChildren();
        mSplit = nullptr;
        mSplitHandle = nullptr;
        mHint = nullptr;
        mResizePane = false;
        if (mDoc == nullptr || !mDoc->isOpen())
        {
            return;
        }

        const float s = this->scale();
        const float fontPx = 15.0f * s;
        const RoseTheme& t = theme();
        const Color canvas = t.canvas;
        const Color mute = t.mute;
        const Color line = t.line;
        const String& fontPath = this->font();
        this->color = canvas;
        const float height = shape.size.y;
        float outlinerW = 0.0f;
        float paneX = 0.0f;
        float paneW = 0.0f;
        this->paneRects(outlinerW, paneX, paneW);

        if (!mOutliner)
        {
            mOutliner = std::make_shared<RoseSchemaOutliner>();
        }
        mOutliner->bind(*mDoc, this->outlinerHost());
        mOutliner->shape.pivot = Vector2(0.0f, 0.0f);
        setBox(mOutliner, Rect(0.0f, 0.0f, outlinerW, height));
        mOutliner->setOverlay(mOverlay);
        mOutliner->setScroll(mKeepOutlinerY);
        mOutliner->rebuild();
        this->attachChild(mOutliner);

        auto split = std::make_shared<UIWidget>();
        split->pickable = false;
        split->color = line;
        split->shape.pivot = Vector2(0.0f, 0.0f);
        setBox(split, Rect(outlinerW, 0.0f, 1.0f, height));
        this->attachChild(split);
        mSplit = split.get();
        this->attachPaneHandle();

        if (!mDoc->hasActiveSchema())
        {
            auto card = std::make_shared<PaintPanel>();
            card->pickable = false;
            card->fill(t.cardTop, t.cardBottom);
            card->hair = t.hair;
            card->framed = true;
            float cardW = paneW - 32.0f * s;
            if (cardW > 320.0f * s)
            {
                cardW = 320.0f * s;
            }
            if (cardW < 1.0f)
            {
                cardW = 1.0f;
            }
            auto hint = makeText("选择或新建模式", fontPath, fontPx, mute, cardW - 32.0f * s, 28.0f * s);
            hint->layout();
            float cardH = 48.0f * s + hint->shape.size.y;
            setBox(card, Rect(paneX + (paneW - cardW) * 0.5f, (height - cardH) * 0.5f, cardW, cardH));
            setBox(hint, Rect((cardW - hint->shape.size.x) * 0.5f, (cardH - hint->shape.size.y) * 0.5f, hint->shape.size.x, hint->shape.size.y));
            card->attachChild(hint);
            this->attachChild(card);
            mHint = card.get();
            return;
        }

        if (!mSchemaEditor)
        {
            mSchemaEditor = std::make_shared<RoseSchemaEditor>();
        }
        mSchemaEditor->bind(*mDoc, this->editorHost());
        mSchemaEditor->shape.pivot = Vector2(0.0f, 0.0f);
        setBox(mSchemaEditor, Rect(paneX, 0.0f, paneW, height));
        mSchemaEditor->setOverlay(mOverlay);
        mSchemaEditor->setScroll(mKeepEditorY);
        mSchemaEditor->rebuild();
        this->attachChild(mSchemaEditor);
    }

}
