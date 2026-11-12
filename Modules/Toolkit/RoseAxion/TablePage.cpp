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
        if (mDetails)
        {
            mDetails->setOverlay(overlay);
        }
        mOverlay = overlay;
    }

    const String& RoseTablePage::font() const
    {
        return mHost.font;
    }

    float RoseTablePage::scale() const
    {
        return hostScale(mHost.scale);
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
        host.openObject = [this](int row)
        {
            if (mDetails)
            {
                mDetails->openObject(row);
            }
        };
        host.openField = [this](int col)
        {
            if (mDetails)
            {
                mDetails->openField(col);
            }
        };
        host.noteRowDeleted = [this](int row)
        {
            if (mDetails)
            {
                mDetails->noteRowDeleted(row);
            }
        };
        host.noteRowInserted = [this](int index)
        {
            if (mDetails)
            {
                mDetails->noteRowInserted(index);
            }
        };
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

    RoseTableDetails::Host RoseTablePage::detailsHost() const
    {
        RoseTableDetails::Host host;
        host.requestRebuild = mHost.requestRebuild;
        host.writeClipboard = mHost.writeClipboard;
        host.readClipboard = mHost.readClipboard;
        host.openExternal = mHost.openExternal;
        host.pickFilePath = mHost.pickFilePath;
        host.pickFolderPath = mHost.pickFolderPath;
        host.ctrlDown = mHost.ctrlDown;
        host.confirmChangeType = mHost.confirmChangeType;
        host.confirmDeleteColumn = mHost.confirmDeleteColumn;
        host.overlay = mOverlay;
        host.font = mHost.font;
        host.scale = mHost.scale;
        return host;
    }

    void RoseTablePage::reset()
    {
        mResizeSide = PaneResize::None;
        this->setOverlay(nullptr);
        if (mTableEditor)
        {
            mTableEditor->reset();
        }
        if (mDetails)
        {
            mDetails->reset();
        }
        mOutlinerSplit = nullptr;
        mDetailsSplit = nullptr;
        mOutlinerHandle = nullptr;
        mDetailsHandle = nullptr;
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
        if (mDetails)
        {
            mDetails->close();
        }
    }

    UIWidget* RoseTablePage::focusWidget() const
    {
        if (mTableEditor != nullptr && mTableEditor->hasPopupEdit())
        {
            if (UIWidget* focus = mTableEditor->focusWidget())
            {
                return focus;
            }
        }
        if (mDetails != nullptr && mDetails->hasPopupEdit())
        {
            if (UIWidget* focus = mDetails->focusWidget())
            {
                return focus;
            }
        }
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

    void RoseTablePage::paneRects(float& outlinerW, float& tableX, float& tableW, float& detailsX, float& detailsW) const
    {
        const float s = this->scale();
        const float split = 1.0f;
        const float minOutliner = 160.0f * s;
        const float minDetails = 200.0f * s;
        const float minTable = 80.0f * s;
        float available = shape.size.x - 2.0f * split;
        if (available < 40.0f)
        {
            available = 40.0f;
        }

        outlinerW = mOutlinerW * s;
        detailsW = mDetailsW * s;
        if (outlinerW < minOutliner)
        {
            outlinerW = minOutliner;
        }
        if (detailsW < minDetails)
        {
            detailsW = minDetails;
        }

        float leftover = available - outlinerW - detailsW;
        if (leftover < minTable)
        {
            float overflow = minTable - leftover;
            float detailsSlack = detailsW - minDetails;
            if (detailsSlack < 0.0f)
            {
                detailsSlack = 0.0f;
            }
            if (overflow > 0.0f && detailsSlack > 0.0f)
            {
                float cut = overflow < detailsSlack ? overflow : detailsSlack;
                detailsW -= cut;
                overflow -= cut;
            }
            float outlinerSlack = outlinerW - minOutliner;
            if (outlinerSlack < 0.0f)
            {
                outlinerSlack = 0.0f;
            }
            if (overflow > 0.0f && outlinerSlack > 0.0f)
            {
                float cut = overflow < outlinerSlack ? overflow : outlinerSlack;
                outlinerW -= cut;
            }
            leftover = available - outlinerW - detailsW;
            if (leftover < 40.0f)
            {
                leftover = 40.0f;
            }
        }
        tableW = leftover;
        tableX = outlinerW + split;
        detailsX = tableX + tableW + split;
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
        float detailsX = 0.0f;
        float detailsW = 0.0f;
        this->paneRects(outlinerW, tableX, tableW, detailsX, detailsW);
        const float height = shape.size.y;
        if (mOutliner != nullptr)
        {
            setBox(*mOutliner, Rect(0.0f, 0.0f, outlinerW, height));
        }
        if (mOutlinerSplit != nullptr)
        {
            setBox(*mOutlinerSplit, Rect(outlinerW, 0.0f, 1.0f, height));
        }
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
        if (mDetailsSplit != nullptr)
        {
            setBox(*mDetailsSplit, Rect(detailsX - 1.0f, 0.0f, 1.0f, height));
        }
        if (mDetails != nullptr)
        {
            setBox(*mDetails, Rect(detailsX, 0.0f, detailsW, height));
            mDetails->layout();
        }
        this->placePaneHandles();
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
        if (mResizeSide != PaneResize::None)
        {
            this->cancelResize();
            return true;
        }
        if (mTableEditor != nullptr && mTableEditor->handleEscape())
        {
            return true;
        }
        return mDetails != nullptr && mDetails->handleEscape();
    }

    bool RoseTablePage::colorDropping() const
    {
        if (mDetails != nullptr && mDetails->colorDropping())
        {
            return true;
        }
        return mTableEditor != nullptr && mTableEditor->colorDropping();
    }

    void RoseTablePage::sampleColorDropper(float x, float y, bool commit)
    {
        if (mDetails != nullptr && mDetails->colorDropping())
        {
            mDetails->sampleColorDropper(x, y, commit);
            return;
        }
        if (mTableEditor)
        {
            mTableEditor->sampleColorDropper(x, y, commit);
        }
    }

    void RoseTablePage::cancelColorDropper()
    {
        if (mDetails)
        {
            mDetails->cancelColorDropper();
        }
        if (mTableEditor)
        {
            mTableEditor->cancelColorDropper();
        }
    }

    bool RoseTablePage::resizeCursorAt(float x, float y) const
    {
        if (mResizeSide != PaneResize::None)
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
        auto hitHandle = [](UIWidget* handle, float x, float y) -> bool
        {
            if (handle == nullptr)
            {
                return false;
            }
            Vector2 local = handle->screenToLocal(Vector2(x, y));
            return handle->contains(local);
        };
        if (hitHandle(mOutlinerHandle, screenX, screenY) || hitHandle(mDetailsHandle, screenX, screenY))
        {
            return true;
        }
        Vector2 local = this->screenToLocal(Vector2(screenX, screenY));
        if (local.y < 0.0f || local.y > shape.size.y)
        {
            return false;
        }
        const float grip = 6.0f * this->scale();
        float outlinerW = 0.0f;
        float tableX = 0.0f;
        float tableW = 0.0f;
        float detailsX = 0.0f;
        float detailsW = 0.0f;
        this->paneRects(outlinerW, tableX, tableW, detailsX, detailsW);
        if (local.x >= outlinerW - grip * 0.5f && local.x <= outlinerW + grip * 0.5f)
        {
            return true;
        }
        return local.x >= detailsX - grip * 0.5f && local.x <= detailsX + grip * 0.5f;
    }

    void RoseTablePage::placePaneHandles()
    {
        const float grip = 6.0f * this->scale();
        float outlinerW = 0.0f;
        float tableX = 0.0f;
        float tableW = 0.0f;
        float detailsX = 0.0f;
        float detailsW = 0.0f;
        this->paneRects(outlinerW, tableX, tableW, detailsX, detailsW);
        if (mOutlinerHandle != nullptr)
        {
            setBox(*mOutlinerHandle, Rect(outlinerW - grip * 0.5f, 0.0f, grip, shape.size.y));
        }
        if (mDetailsHandle != nullptr)
        {
            setBox(*mDetailsHandle, Rect(detailsX - grip * 0.5f, 0.0f, grip, shape.size.y));
        }
    }

    void RoseTablePage::applyOutlinerWidth(float pixelWidth)
    {
        const float s = this->scale();
        mOutlinerW = pixelWidth / s;
        float outlinerW = 0.0f;
        float tableX = 0.0f;
        float tableW = 0.0f;
        float detailsX = 0.0f;
        float detailsW = 0.0f;
        this->paneRects(outlinerW, tableX, tableW, detailsX, detailsW);
        mOutlinerW = outlinerW / s;
        this->layout();
        if (mOutliner != nullptr)
        {
            mKeepOutlinerY = mOutliner->scrollY();
            mOutliner->setScroll(mKeepOutlinerY);
            mOutliner->rebuild();
            mOutliner->layout();
        }
    }

    void RoseTablePage::applyDetailsWidth(float pixelWidth)
    {
        const float s = this->scale();
        mDetailsW = pixelWidth / s;
        float outlinerW = 0.0f;
        float tableX = 0.0f;
        float tableW = 0.0f;
        float detailsX = 0.0f;
        float detailsW = 0.0f;
        this->paneRects(outlinerW, tableX, tableW, detailsX, detailsW);
        mDetailsW = detailsW / s;
        this->layout();
        if (mDetails != nullptr)
        {
            mDetails->rebuild();
            mDetails->layout();
        }
    }

    void RoseTablePage::commitPaneResize()
    {
        mResizeSide = PaneResize::None;
    }

    void RoseTablePage::attachSplitHandle(PaneResize side)
    {
        const float grip = 6.0f * this->scale();
        float outlinerW = 0.0f;
        float tableX = 0.0f;
        float tableW = 0.0f;
        float detailsX = 0.0f;
        float detailsW = 0.0f;
        this->paneRects(outlinerW, tableX, tableW, detailsX, detailsW);
        float x = side == PaneResize::Outliner ? outlinerW : detailsX;
        auto handle = std::make_shared<UIWidget>();
        handle->overlay = true;
        handle->pickable = true;
        handle->color = Color(0.0f, 0.0f, 0.0f, 0.0f);
        handle->shape.pivot = Vector2(0.0f, 0.0f);
        setBox(handle, Rect(x - grip * 0.5f, 0.0f, grip, shape.size.y));
        handle->onPointerPress = [this, side](const UIPointerEvent& event)
        {
            if (event.pointerCode != UIPointerCode::Left)
            {
                return;
            }
            mResizeSide = side;
            float outlinerW = 0.0f;
            float tableX = 0.0f;
            float tableW = 0.0f;
            float detailsX = 0.0f;
            float detailsW = 0.0f;
            this->paneRects(outlinerW, tableX, tableW, detailsX, detailsW);
            mResizeOriginPaneW = side == PaneResize::Outliner ? outlinerW : detailsW;
        };
        handle->onDrag = [this, side](const Vector2&, const Vector2& delta)
        {
            if (mResizeSide != side)
            {
                return;
            }
            if (side == PaneResize::Outliner)
            {
                this->applyOutlinerWidth(mResizeOriginPaneW + delta.x);
            }
            else
            {
                this->applyDetailsWidth(mResizeOriginPaneW - delta.x);
            }
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
        if (side == PaneResize::Outliner)
        {
            mOutlinerHandle = handle.get();
        }
        else
        {
            mDetailsHandle = handle.get();
        }
    }

    void RoseTablePage::cancelResize()
    {
        if (mResizeSide == PaneResize::Outliner)
        {
            this->applyOutlinerWidth(mResizeOriginPaneW);
        }
        else if (mResizeSide == PaneResize::Details)
        {
            this->applyDetailsWidth(mResizeOriginPaneW);
        }
        mResizeSide = PaneResize::None;
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
        if (mDetails)
        {
            mDetails->setOverlay(nullptr);
        }
        this->detachChildren();
        mOutlinerSplit = nullptr;
        mDetailsSplit = nullptr;
        mOutlinerHandle = nullptr;
        mDetailsHandle = nullptr;
        mHint = nullptr;
        mResizeSide = PaneResize::None;
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
        float tableX = 0.0f;
        float tableW = 0.0f;
        float detailsX = 0.0f;
        float detailsW = 0.0f;
        this->paneRects(outlinerW, tableX, tableW, detailsX, detailsW);

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

        auto outlinerSplit = std::make_shared<UIWidget>();
        outlinerSplit->pickable = false;
        outlinerSplit->color = line;
        outlinerSplit->shape.pivot = Vector2(0.0f, 0.0f);
        setBox(outlinerSplit, Rect(outlinerW, 0.0f, 1.0f, height));
        this->attachChild(outlinerSplit);
        mOutlinerSplit = outlinerSplit.get();

        if (!mDoc->hasActiveTable())
        {
            auto hint = makeText("选择或新建表格", fontPath, fontPx, mute, tableW - 32.0f * s, 28.0f * s);
            hint->layout();
            setBox(hint, Rect(tableX + (tableW - hint->shape.size.x) * 0.5f, (height - hint->shape.size.y) * 0.5f, hint->shape.size.x, hint->shape.size.y));
            this->attachChild(hint);
            mHint = hint.get();
        }
        else
        {
            if (!mTableEditor)
            {
                mTableEditor = std::make_shared<RoseTableEditor>();
            }
            mTableEditor->bind(*mDoc, this->tableHost());
            mTableEditor->shape.pivot = Vector2(0.0f, 0.0f);
            setBox(mTableEditor, Rect(tableX, 0.0f, tableW, height));
            mTableEditor->setOverlay(mOverlay);
            mTableEditor->rebuild();
            this->attachChild(mTableEditor);
        }

        auto detailsSplit = std::make_shared<UIWidget>();
        detailsSplit->pickable = false;
        detailsSplit->color = line;
        detailsSplit->shape.pivot = Vector2(0.0f, 0.0f);
        setBox(detailsSplit, Rect(detailsX - 1.0f, 0.0f, 1.0f, height));
        this->attachChild(detailsSplit);
        mDetailsSplit = detailsSplit.get();

        if (!mDetails)
        {
            mDetails = std::make_shared<RoseTableDetails>();
        }
        mDetails->bind(*mDoc, this->detailsHost());
        mDetails->shape.pivot = Vector2(0.0f, 0.0f);
        setBox(mDetails, Rect(detailsX, 0.0f, detailsW, height));
        mDetails->setOverlay(mOverlay);
        mDetails->rebuild();
        this->attachChild(mDetails);

        this->attachSplitHandle(PaneResize::Outliner);
        this->attachSplitHandle(PaneResize::Details);
    }

}
