#include "./Editor.h"
#include "./Theme.h"
#include "./UiKit.h"

#include "UI/Main.h"

namespace eokas::RoseAxion {

    void RoseEditor::ensurePages()
    {
        if (!mNavigator)
        {
            mNavigator = std::make_shared<RoseNavigator>();
        }
        if (!mHomePage)
        {
            mHomePage = std::make_shared<RoseHomePage>();
        }
        if (!mInstancePage)
        {
            mInstancePage = std::make_shared<RoseInstancePage>();
        }
        if (!mSchemaPage)
        {
            mSchemaPage = std::make_shared<RoseSchemaPage>();
        }
        if (!mSearchPage)
        {
            mSearchPage = std::make_shared<RoseSearchPage>();
        }
        if (!mSettingsPage)
        {
            mSettingsPage = std::make_shared<RoseSettingsPage>();
        }
        if (mDoc != nullptr)
        {
            mHomePage->bind(*mDoc, this->homeHost());
            mInstancePage->bind(*mDoc, this->instanceHost());
            mSchemaPage->bind(*mDoc, this->schemaHost());
        }
        mSearchPage->bind(this->searchHost());
        mSettingsPage->bind(this->settingsHost());
        mNavigator->bind(this->navigatorHost());
    }

    RoseNavigator::Host RoseEditor::navigatorHost()
    {
        RoseNavigator::Host host;
        host.current = mPageKind;
        host.font = mHost.font;
        host.scale = mHost.scale;
        host.selectPage = [this](EditorPage page) { this->setPage(page); };
        return host;
    }

    RoseHomePage::Host RoseEditor::homeHost()
    {
        RoseHomePage::Host host;
        host.requestRebuild = mHost.requestRebuild;
        host.isRebuildingUi = mHost.isRebuildingUi;
        host.selectPage = [this](EditorPage page) { this->setPage(page); };
        host.font = mHost.font;
        host.scale = mHost.scale;
        return host;
    }

    RoseInstancePage::Host RoseEditor::instanceHost()
    {
        RoseInstancePage::Host host;
        host.requestRebuild = mHost.requestRebuild;
        host.isRebuildingUi = mHost.isRebuildingUi;
        host.writeClipboard = mHost.writeClipboard;
        host.readClipboard = mHost.readClipboard;
        host.openExternal = mHost.openExternal;
        host.pickFilePath = mHost.pickFilePath;
        host.pickFolderPath = mHost.pickFolderPath;
        host.ctrlDown = mHost.ctrlDown;
        host.confirmDeleteInstance = mHost.confirmDeleteInstance;
        host.overlay = [this]() { return mOverlay; };
        host.sharedOutlinerW = &mPaneOutlinerW;
        host.font = mHost.font;
        host.scale = mHost.scale;
        return host;
    }

    RoseSchemaPage::Host RoseEditor::schemaHost()
    {
        RoseSchemaPage::Host host;
        host.requestRebuild = mHost.requestRebuild;
        host.isRebuildingUi = mHost.isRebuildingUi;
        host.writeClipboard = mHost.writeClipboard;
        host.readClipboard = mHost.readClipboard;
        host.confirmDeleteSchema = mHost.confirmDeleteSchema;
        host.sharedOutlinerW = &mPaneOutlinerW;
        host.font = mHost.font;
        host.scale = mHost.scale;
        return host;
    }

    RoseSearchPage::Host RoseEditor::searchHost() const
    {
        RoseSearchPage::Host host;
        host.font = mHost.font;
        host.scale = mHost.scale;
        return host;
    }

    RoseSettingsPage::Host RoseEditor::settingsHost() const
    {
        RoseSettingsPage::Host host;
        host.requestRebuild = mHost.requestRebuild;
        host.isRebuildingUi = mHost.isRebuildingUi;
        host.font = mHost.font;
        host.scale = mHost.scale;
        return host;
    }

    void RoseEditor::setPage(EditorPage page)
    {
        if (mPageKind == page)
        {
            return;
        }
        mPageKind = page;
        if (mHost.requestRebuild)
        {
            mHost.requestRebuild();
        }
    }

    UIWidget* RoseEditor::activePage() const
    {
        if (mPageKind == EditorPage::Home)
        {
            return mHomePage.get();
        }
        if (mPageKind == EditorPage::Instance)
        {
            return mInstancePage.get();
        }
        if (mPageKind == EditorPage::Schemas)
        {
            return mSchemaPage.get();
        }
        if (mPageKind == EditorPage::Search)
        {
            return mSearchPage.get();
        }
        if (mPageKind == EditorPage::Settings)
        {
            return mSettingsPage.get();
        }
        return mInstancePage.get();
    }

    void RoseEditor::bind(TableDocument& doc, const Host& host)
    {
        mDoc = &doc;
        mHost = host;
        this->ensurePages();
    }

    void RoseEditor::reset()
    {
        if (mHomePage)
        {
            mHomePage->reset();
        }
        if (mInstancePage)
        {
            mInstancePage->setOverlay(nullptr);
            mInstancePage->reset();
        }
        mOverlay = nullptr;
        this->detachChildren();
    }

    void RoseEditor::resetOutliner()
    {
        if (mSchemaPage)
        {
            mSchemaPage->resetOutliner();
        }
        if (mInstancePage)
        {
            mInstancePage->resetOutliner();
        }
    }

    void RoseEditor::resetSchema()
    {
        if (mSchemaPage)
        {
            mSchemaPage->setOverlay(nullptr);
            mSchemaPage->reset();
            mSchemaPage->resetOutliner();
        }
    }

    UIWidget* RoseEditor::focusWidget() const
    {
        if (mPageKind == EditorPage::Schemas && mSchemaPage)
        {
            return mSchemaPage->focusWidget();
        }
        if (mPageKind == EditorPage::Instance && mInstancePage)
        {
            return mInstancePage->focusWidget();
        }
        return nullptr;
    }

    void RoseEditor::placeChildren()
    {
        const float navW = mNavigator != nullptr ? mNavigator->barWidth() : 0.0f;
        const float width = shape.size.x;
        const float height = shape.size.y;
        float pageW = width - navW;
        if (pageW < 40.0f)
        {
            pageW = 40.0f;
        }
        if (mNavigator != nullptr)
        {
            mNavigator->shape.pivot = Vector2(0.0f, 0.0f);
            setBox(*mNavigator, Rect(0.0f, 0.0f, navW, height));
        }
        UIWidget* page = this->activePage();
        if (page != nullptr)
        {
            page->shape.pivot = Vector2(0.0f, 0.0f);
            setBox(*page, Rect(navW, 0.0f, pageW, height));
        }
        if (mOverlay != nullptr)
        {
            setBox(*mOverlay, Rect(0.0f, 0.0f, width, height));
        }
    }

    void RoseEditor::layout()
    {
        if (!visible)
        {
            return;
        }
        this->placeChildren();
        if (mOverlay != nullptr)
        {
            setBox(*mOverlay, Rect(0.0f, 0.0f, shape.size.x, shape.size.y));
        }
        if (mNavigator != nullptr)
        {
            mNavigator->layout();
        }
        if (mPageKind == EditorPage::Home && mHomePage != nullptr)
        {
            mHomePage->layout();
        }
        else if (mPageKind == EditorPage::Instance && mInstancePage != nullptr)
        {
            mInstancePage->layout();
        }
        else if (mPageKind == EditorPage::Schemas && mSchemaPage != nullptr)
        {
            mSchemaPage->layout();
        }
        else if (mPageKind == EditorPage::Search && mSearchPage != nullptr)
        {
            mSearchPage->layout();
        }
        else if (mPageKind == EditorPage::Settings && mSettingsPage != nullptr)
        {
            mSettingsPage->layout();
        }
        UIWidget::layout();
    }

    bool RoseEditor::handleEscape()
    {
        if (mPageKind == EditorPage::Schemas && mSchemaPage)
        {
            return mSchemaPage->handleEscape();
        }
        if (mPageKind == EditorPage::Instance && mInstancePage)
        {
            return mInstancePage->handleEscape();
        }
        return false;
    }

    bool RoseEditor::resizeCursorAt(float x, float y) const
    {
        if (mPageKind == EditorPage::Schemas && mSchemaPage)
        {
            return mSchemaPage->resizeCursorAt(x, y);
        }
        if (mPageKind == EditorPage::Instance && mInstancePage)
        {
            return mInstancePage->resizeCursorAt(x, y);
        }
        return false;
    }

    bool RoseEditor::colorDropping() const
    {
        if (mPageKind == EditorPage::Instance && mInstancePage != nullptr)
        {
            return mInstancePage->colorDropping();
        }
        return false;
    }

    void RoseEditor::sampleColorDropper(float x, float y, bool commit)
    {
        if (mPageKind == EditorPage::Instance && mInstancePage != nullptr)
        {
            mInstancePage->sampleColorDropper(x, y, commit);
        }
    }

    void RoseEditor::cancelColorDropper()
    {
        if (mPageKind == EditorPage::Instance && mInstancePage != nullptr)
        {
            mInstancePage->cancelColorDropper();
        }
    }

    void RoseEditor::rebuild()
    {
        if (mSchemaPage)
        {
            mSchemaPage->setOverlay(nullptr);
        }
        if (mInstancePage)
        {
            mInstancePage->setOverlay(nullptr);
        }
        this->detachChildren();
        mOverlay = nullptr;
        if (mDoc == nullptr || !mDoc->isOpen())
        {
            return;
        }
        this->ensurePages();
        fillSurface(*this, theme().voidBg);
        this->shape.pivot = Vector2(0.0f, 0.0f);

        auto overlay = std::make_shared<UIWidget>();
        overlay->overlay = true;
        overlay->pickable = false;
        overlay->shape.pivot = Vector2(0.0f, 0.0f);
        overlay->color = Color(0.0f, 0.0f, 0.0f, 0.0f);
        mOverlay = overlay.get();
        this->placeChildren();

        mNavigator->rebuild();
        if (mPageKind == EditorPage::Home)
        {
            mHomePage->rebuild();
        }
        else if (mPageKind == EditorPage::Instance)
        {
            mInstancePage->setOverlay(mOverlay);
            mInstancePage->rebuild();
        }
        else if (mPageKind == EditorPage::Schemas)
        {
            mSchemaPage->setOverlay(mOverlay);
            mSchemaPage->rebuild();
        }
        else if (mPageKind == EditorPage::Search)
        {
            mSearchPage->rebuild();
        }
        else
        {
            mSettingsPage->rebuild();
        }

        if (mNavigator)
        {
            this->attachChild(mNavigator);
        }
        if (mPageKind == EditorPage::Home && mHomePage)
        {
            this->attachChild(mHomePage);
        }
        else if (mPageKind == EditorPage::Instance && mInstancePage)
        {
            this->attachChild(mInstancePage);
        }
        else if (mPageKind == EditorPage::Schemas && mSchemaPage)
        {
            this->attachChild(mSchemaPage);
        }
        else if (mPageKind == EditorPage::Search && mSearchPage)
        {
            this->attachChild(mSearchPage);
        }
        else if (mPageKind == EditorPage::Settings && mSettingsPage)
        {
            this->attachChild(mSettingsPage);
        }
        this->attachChild(overlay);
    }

}