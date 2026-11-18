#include "./InstancePage.h"
#include "./Theme.h"
#include "./UiKit.h"

#include "UI/Main.h"
#include "UI/UIFont.h"

namespace eokas::RoseAxion {

    void RoseInstancePage::bind(TableDocument& doc, const Host& host)
    {
        mDoc = &doc;
        mHost = host;
        if (mHost.sharedOutlinerW != nullptr)
        {
            mOutlinerW = *mHost.sharedOutlinerW;
        }
    }

    void RoseInstancePage::setOverlay(UIWidget* overlay)
    {
        if (mOutliner)
        {
            mOutliner->setOverlay(overlay);
        }
        if (mDesigner)
        {
            mDesigner->setOverlay(overlay);
        }
        mOverlay = overlay;
    }

    const String& RoseInstancePage::font() const
    {
        return mHost.font;
    }

    float RoseInstancePage::scale() const
    {
        return hostScale(mHost.scale);
    }

    RoseInstanceDesigner::Host RoseInstancePage::designerHost() const
    {
        RoseInstanceDesigner::Host host;
        host.requestRebuild = mHost.requestRebuild;
        host.isRebuildingUi = mHost.isRebuildingUi;
        host.writeClipboard = mHost.writeClipboard;
        host.readClipboard = mHost.readClipboard;
        host.openExternal = mHost.openExternal;
        host.pickFilePath = mHost.pickFilePath;
        host.pickFolderPath = mHost.pickFolderPath;
        host.ctrlDown = mHost.ctrlDown;
        host.overlay = mOverlay;
        host.font = mHost.font;
        host.scale = mHost.scale;
        return host;
    }

    RoseInstanceOutliner::Host RoseInstancePage::outlinerHost() const
    {
        RoseInstanceOutliner::Host host;
        host.requestRebuild = mHost.requestRebuild;
        host.isRebuildingUi = mHost.isRebuildingUi;
        host.confirmDeleteInstance = mHost.confirmDeleteInstance;
        host.font = mHost.font;
        host.scale = mHost.scale;
        return host;
    }

    void RoseInstancePage::reset()
    {
        mResizePane = false;
        this->setOverlay(nullptr);
        if (mOutliner)
        {
            mOutliner->reset();
        }
        if (mDesigner)
        {
            mDesigner->reset();
        }
        mSplit = nullptr;
        mSplitHandle = nullptr;
        mHint = nullptr;
        this->detachChildren();
    }

    void RoseInstancePage::resetOutliner()
    {
        mKeepOutlinerY = 0.0f;
        mKeepEditorY = 0.0f;
        if (mOutliner)
        {
            mOutliner->setOverlay(nullptr);
            mOutliner->reset();
        }
        if (mDesigner)
        {
            mDesigner->setOverlay(nullptr);
            mDesigner->reset();
        }
    }

    UIWidget* RoseInstancePage::focusWidget() const
    {
        if (mOutliner != nullptr)
        {
            if (UIWidget* focus = mOutliner->focusWidget())
            {
                return focus;
            }
        }
        return mDesigner != nullptr ? mDesigner->focusWidget() : nullptr;
    }

    float RoseInstancePage::outlinerWidth() const
    {
        const float s = this->scale();
        const float minW = kRoseOutlinerMinW * s;
        float maxW = shape.size.x - kRoseOutlinerEditorMinW * s;
        if (maxW < minW)
        {
            maxW = minW;
        }
        const float logicalW = mHost.sharedOutlinerW != nullptr ? *mHost.sharedOutlinerW : mOutlinerW;
        float width = logicalW * s;
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

    void RoseInstancePage::paneRects(float& outlinerW, float& editorX, float& editorW) const
    {
        outlinerW = this->outlinerWidth();
        editorX = outlinerW + 1.0f;
        editorW = shape.size.x - editorX;
        if (editorW < 40.0f)
        {
            editorW = 40.0f;
        }
    }

    void RoseInstancePage::layout()
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
        if (mDesigner != nullptr && mDesigner->parent() == this)
        {
            setBox(*mDesigner, Rect(editorX, 0.0f, editorW, height));
            mDesigner->layout();
        }
        if (mHint != nullptr)
        {
            setBox(*mHint, Rect(editorX, 0.0f, editorW, height));
            if (!mHint->children().empty() && mHint->children()[0])
            {
                UIWidget& card = *mHint->children()[0];
                card.layout();
                setBox(card, Rect((editorW - card.shape.size.x) * 0.5f, (height - card.shape.size.y) * 0.5f, card.shape.size.x, card.shape.size.y));
            }
        }
        if (mOutliner != nullptr)
        {
            mOutliner->layout();
        }
        UIWidget::layout();
    }

    bool RoseInstancePage::handleEscape()
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
        return mDesigner != nullptr && mDesigner->handleEscape();
    }

    bool RoseInstancePage::colorDropping() const
    {
        return mDesigner != nullptr && mDesigner->colorDropping();
    }

    void RoseInstancePage::sampleColorDropper(float x, float y, bool commit)
    {
        if (mDesigner != nullptr)
        {
            mDesigner->sampleColorDropper(x, y, commit);
        }
    }

    void RoseInstancePage::cancelColorDropper()
    {
        if (mDesigner != nullptr)
        {
            mDesigner->cancelColorDropper();
        }
    }

    bool RoseInstancePage::resizeCursorAt(float x, float y) const
    {
        if (mResizePane)
        {
            return true;
        }
        return this->hitResizePane(x, y);
    }

    bool RoseInstancePage::hitResizePane(float screenX, float screenY) const
    {
        if (mSplitHandle != nullptr)
        {
            Vector2 local = mSplitHandle->screenToLocal(Vector2(screenX, screenY));
            return mSplitHandle->contains(local);
        }
        Vector2 local = this->screenToLocal(Vector2(screenX, screenY));
        const float grip = kRosePaneGrip * this->scale();
        float outlinerW = this->outlinerWidth();
        return local.y >= 0.0f && local.y <= shape.size.y
            && local.x >= outlinerW - grip * 0.5f && local.x <= outlinerW + grip * 0.5f;
    }

    void RoseInstancePage::placePaneHandle()
    {
        if (mSplitHandle == nullptr)
        {
            return;
        }
        const float grip = kRosePaneGrip * this->scale();
        float outlinerW = this->outlinerWidth();
        setBox(*mSplitHandle, Rect(outlinerW - grip * 0.5f, 0.0f, grip, shape.size.y));
    }

    void RoseInstancePage::applyPaneWidth(float pixelWidth)
    {
        const float s = this->scale();
        const float minW = kRoseOutlinerMinW * s;
        float maxW = shape.size.x - kRoseOutlinerEditorMinW * s;
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
        if (mHost.sharedOutlinerW != nullptr)
        {
            *mHost.sharedOutlinerW = mOutlinerW;
        }
        this->layout();
        if (mOutliner != nullptr)
        {
            mKeepOutlinerY = mOutliner->scrollY();
            mOutliner->setScroll(mKeepOutlinerY);
            mOutliner->rebuild();
            mOutliner->layout();
        }
        if (mDesigner != nullptr && mDesigner->parent() == this)
        {
            mKeepEditorY = mDesigner->scrollY();
            mDesigner->setScroll(mKeepEditorY);
            mDesigner->rebuild();
            mDesigner->layout();
        }
    }

    void RoseInstancePage::commitPaneResize()
    {
        mResizePane = false;
    }

    void RoseInstancePage::attachPaneHandle()
    {
        const float grip = kRosePaneGrip * this->scale();
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

    void RoseInstancePage::cancelResize()
    {
        if (mResizePane)
        {
            this->applyPaneWidth(mResizeOriginPaneW);
        }
        mResizePane = false;
    }

    void RoseInstancePage::rebuild()
    {
        if (mOutliner)
        {
            mKeepOutlinerY = mOutliner->scrollY();
            mOutliner->setOverlay(nullptr);
        }
        if (mDesigner)
        {
            mKeepEditorY = mDesigner->scrollY();
            mDesigner->setOverlay(nullptr);
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
        const float fontPx = bodyPx(s);
        const RoseTheme& t = theme();
        const Color mute = t.mute;
        const String& fontPath = this->font();
        fillSurface(*this, t.voidBg);
        const float height = shape.size.y;
        float outlinerW = 0.0f;
        float paneX = 0.0f;
        float paneW = 0.0f;
        this->paneRects(outlinerW, paneX, paneW);

        if (!mOutliner)
        {
            mOutliner = std::make_shared<RoseInstanceOutliner>();
        }
        mOutliner->bind(*mDoc, this->outlinerHost());
        mOutliner->shape.pivot = Vector2(0.0f, 0.0f);
        setBox(mOutliner, Rect(0.0f, 0.0f, outlinerW, height));
        mOutliner->setOverlay(mOverlay);
        mOutliner->setScroll(mKeepOutlinerY);
        mOutliner->rebuild();
        this->attachChild(mOutliner);

        auto split = makeSplit(height, true);
        setBox(split, Rect(outlinerW, 0.0f, 1.0f, height));
        this->attachChild(split);
        mSplit = split.get();
        this->attachPaneHandle();

        if (!mDoc->hasActiveInstance())
        {
            auto stage = std::make_shared<UIWidget>();
            stage->shape.pivot = Vector2(0.0f, 0.0f);
            stage->pickable = false;
            fillSurface(*stage, t.paper);
            setBox(stage, Rect(paneX, 0.0f, paneW, height));
            auto card = std::make_shared<PaintPanel>();
            card->pickable = false;
            card->fill(t.paper);
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
            auto hint = makeText("Select or create an instance", fontPath, fontPx, mute, cardW - 32.0f * s, 28.0f * s);
            hint->layout();
            float cardH = 48.0f * s + hint->shape.size.y;
            setBox(card, Rect((paneW - cardW) * 0.5f, (height - cardH) * 0.5f, cardW, cardH));
            setBox(hint, Rect((cardW - hint->shape.size.x) * 0.5f, (cardH - hint->shape.size.y) * 0.5f, hint->shape.size.x, hint->shape.size.y));
            card->attachChild(hint);
            stage->attachChild(card);
            this->attachChild(stage);
            mHint = stage.get();
            return;
        }

        if (!mDesigner)
        {
            mDesigner = std::make_shared<RoseInstanceDesigner>();
        }
        mDesigner->bind(*mDoc, this->designerHost());
        mDesigner->shape.pivot = Vector2(0.0f, 0.0f);
        setBox(mDesigner, Rect(paneX, 0.0f, paneW, height));
        mDesigner->setOverlay(mOverlay);
        mDesigner->setScroll(mKeepEditorY);
        mDesigner->rebuild();
        this->attachChild(mDesigner);
    }

}
