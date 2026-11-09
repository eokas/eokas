#ifndef _EOKAS_ROSE_AXION_EDITOR_H_
#define _EOKAS_ROSE_AXION_EDITOR_H_

#include "./Document.h"
#include "./HomePage.h"
#include "./Navigator.h"
#include "./SchemaPage.h"
#include "./SearchPage.h"
#include "./SettingsPage.h"
#include "./TablePage.h"

#include "UI/UIEvent.h"
#include "UI/UIWidget.h"

#include <memory>

namespace eokas::RoseAxion {

    class RoseEditor : public UIWidget
    {
    public:
        using Host = RoseTablePage::Host;

        void bind(TableDocument& doc, const Host& host);
        void reset();
        void resetOutliner();
        void resetSchema();
        void selectTables();
        void closeSettings();
        void rebuild();
        bool resizeCursorAt(float x, float y) const;
        bool handleEscape();
        bool colorDropping() const;
        void sampleColorDropper(float x, float y, bool commit);
        void cancelColorDropper();
        float scrollX() const;
        float scrollY() const;
        void setScroll(float x, float y);
        UIWidget* focusWidget() const;

        void layout() override;

    private:
        void ensurePages();
        void setPage(EditorPage page);
        UIWidget* activePage() const;
        RoseNavigator::Host navigatorHost();
        RoseHomePage::Host homeHost();
        RoseSchemaPage::Host schemaHost() const;
        RoseSearchPage::Host searchHost() const;
        RoseSettingsPage::Host settingsHost() const;
        void placeChildren();

        TableDocument* mDoc = nullptr;
        Host mHost;
        EditorPage mPageKind = EditorPage::Tables;
        std::shared_ptr<RoseNavigator> mNavigator;
        std::shared_ptr<RoseHomePage> mHomePage;
        std::shared_ptr<RoseTablePage> mTablePage;
        std::shared_ptr<RoseSchemaPage> mSchemaPage;
        std::shared_ptr<RoseSearchPage> mSearchPage;
        std::shared_ptr<RoseSettingsPage> mSettingsPage;
        UIWidget* mOverlay = nullptr;
    };

}

#endif
