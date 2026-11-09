#include "./TablePage.h"
#include "./Theme.h"
#include "./UiKit.h"

#include "UI/Main.h"
#include "UI/UIFont.h"

namespace eokas::RoseAxion {

    void RoseTablePage::bind(TableDocument& doc, const Host& host)
    {
        mDoc = &doc;
        mHost = host;
    }

    void RoseTablePage::setOverlay(UIWidget* overlay)
    {
        if (mOutliner)
        {
            mOutliner->setOverlay(overlay);
        }
        if (mTableEditor)
        {
            mTableEditor->setOverlay(overlay);
        }
        mOverlay = overlay;
    }

    const String& RoseTablePage::font() const
    {
        return mHost.font;
    }

    float RoseTablePage::scale() const
    {
        return mHost.scale < 0.5f ? 1.0f : mHost.scale;
    }

    RoseTableEditor::Host RoseTablePage::tableHost() const
    {
        RoseTableEditor::Host host;
        host.requestRebuild = mHost.requestRebuild;
        host.writeClipboard = mHost.writeClipboard;
        host.readClipboard = mHost.readClipboard;
        host.openExternal = mHost.openExternal;
        host.pickFilePath = mHost.pickFilePath;
        host.pickFolderPath = mHost.pickFolderPath;
        host.ctrlDown = mHost.ctrlDown;
        host.handleShortcut = mHost.handleShortcut;
        host.confirmChangeType = mHost.confirmChangeType;
        host.confirmDeleteColumn = mHost.confirmDeleteColumn;
        host.font = mHost.font;
        host.scale = mHost.scale;
        return host;
    }

    RoseTableOutliner::Host RoseTablePage::outlinerHost() const
    {
        RoseTableOutliner::Host host;
        host.requestRebuild = mHost.requestRebuild;
        host.confirmDeleteTable = mHost.confirmDeleteTable;
        host.font = mHost.font;
        host.scale = mHost.scale;
        return host;
    }

    void RoseTablePage::reset()
    {
        mResizePane = false;
        this->setOverlay(nullptr);
        if (mTableEditor)
        {
            mTableEditor->reset();
        }
        mSplit = nullptr;
        mSplitHandle = nullptr;
        mHint = nullptr;
        this->detachChildren();
    }

    void RoseTablePage::resetOutliner()
    {
        mKeepOutlinerY = 0.0f;
        if (mOutliner)
        {
            mOutliner->setOverlay(nullptr);
            mOutliner->reset();
        }
    }

    void RoseTablePage::closeSettings()
    {
        if (mTableEditor)
        {
            mTableEditor->closeSettings();
        }
    }

    UIWidget* RoseTablePage::focusWidget() const
    {
        if (mOutliner != nullptr)
        {
            if (UIWidget* focus = mOutliner->focusWidget())
            {
                return focus;
            }
        }
        return mTableEditor != nullptr ? mTableEditor->focusWidget() : nullptr;
    }

    float RoseTablePage::scrollX() const
    {
        return mTableEditor != nullptr ? mTableEditor->scrollX() : 0.0f;
    }

    float RoseTablePage::scrollY() const
    {
        return mTableEditor != nullptr ? mTableEditor->scrollY() : 0.0f;
    }

    void RoseTablePage::setScroll(float x, float y)
    {
        if (!mTableEditor)
        {
            mTableEditor = std::make_shared<RoseTableEditor>();
        }
        mTableEditor->setScroll(x, y);
    }

    float RoseTablePage::outlinerWidth() const
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

    void RoseTablePage::paneRects(float& outlinerW, float& tableX, float& tableW) const
    {
        outlinerW = this->outlinerWidth();
        tableX = outlinerW + 1.0f;
        tableW = shape.size.x - tableX;
        if (tableW < 40.0f)
        {
            tableW = 40.0f;
        }
    }

    void RoseTablePage::layout()
    {
        if (!visible)
        {
            return;
        }
        float outlinerW = 0.0f;
        float tableX = 0.0f;
        float tableW = 0.0f;
        this->paneRects(outlinerW, tableX, tableW);
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
        if (mTableEditor != nullptr && mTableEditor->parent() == this)
        {
            setBox(*mTableEditor, Rect(tableX, 0.0f, tableW, height));
            mTableEditor->layout();
        }
        if (mHint != nullptr)
        {
            mHint->layout();
            setBox(*mHint, Rect(tableX + (tableW - mHint->shape.size.x) * 0.5f, (height - mHint->shape.size.y) * 0.5f, mHint->shape.size.x, mHint->shape.size.y));
        }
        if (mOutliner != nullptr)
        {
            mOutliner->layout();
        }
        UIWidget::layout();
    }

    bool RoseTablePage::handleEscape()
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
        return mTableEditor != nullptr && mTableEditor->handleEscape();
    }

    bool RoseTablePage::colorDropping() const
    {
        return mTableEditor != nullptr && mTableEditor->colorDropping();
    }

    void RoseTablePage::sampleColorDropper(float x, float y, bool commit)
    {
        if (mTableEditor)
        {
            mTableEditor->sampleColorDropper(x, y, commit);
        }
    }

    void RoseTablePage::cancelColorDropper()
    {
        if (mTableEditor)
        {
            mTableEditor->cancelColorDropper();
        }
    }

    bool RoseTablePage::resizeCursorAt(float x, float y) const
    {
        if (mResizePane)
        {
            return true;
        }
        if (this->hitResizePane(x, y))
        {
            return true;
        }
        return mTableEditor != nullptr && mTableEditor->resizeCursorAt(x, y);
    }

    bool RoseTablePage::hitResizePane(float screenX, float screenY) const
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

    void RoseTablePage::placePaneHandle()
    {
        if (mSplitHandle == nullptr)
        {
            return;
        }
        const float grip = 6.0f * this->scale();
        float outlinerW = this->outlinerWidth();
        setBox(*mSplitHandle, Rect(outlinerW - grip * 0.5f, 0.0f, grip, shape.size.y));
    }

    void RoseTablePage::applyPaneWidth(float pixelWidth)
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
    }

    void RoseTablePage::commitPaneResize()
    {
        mResizePane = false;
    }

    void RoseTablePage::attachPaneHandle()
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

    void RoseTablePage::cancelResize()
    {
        if (mResizePane)
        {
            this->applyPaneWidth(mResizeOriginPaneW);
        }
        mResizePane = false;
        if (mTableEditor)
        {
            mTableEditor->cancelResize();
        }
    }

    void RoseTablePage::rebuild()
    {
        if (mOutliner)
        {
            mKeepOutlinerY = mOutliner->scrollY();
            mOutliner->setOverlay(nullptr);
        }
        if (mTableEditor)
        {
            mTableEditor->setOverlay(nullptr);
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
            mOutliner = std::make_shared<RoseTableOutliner>();
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

        if (!mDoc->hasActiveTable())
        {
            auto hint = makeText("选择或新建表格", fontPath, fontPx, mute, paneW - 32.0f * s, 28.0f * s);
            hint->layout();
            setBox(hint, Rect(paneX + (paneW - hint->shape.size.x) * 0.5f, (height - hint->shape.size.y) * 0.5f, hint->shape.size.x, hint->shape.size.y));
            this->attachChild(hint);
            mHint = hint.get();
            return;
        }

        if (!mTableEditor)
        {
            mTableEditor = std::make_shared<RoseTableEditor>();
        }
        mTableEditor->bind(*mDoc, this->tableHost());
        mTableEditor->shape.pivot = Vector2(0.0f, 0.0f);
        setBox(mTableEditor, Rect(paneX, 0.0f, paneW, height));
        mTableEditor->setOverlay(mOverlay);
        mTableEditor->rebuild();
        this->attachChild(mTableEditor);
    }

}
