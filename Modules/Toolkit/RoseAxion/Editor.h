#ifndef _EOKAS_ROSE_AXION_EDITOR_H_
#define _EOKAS_ROSE_AXION_EDITOR_H_

#include "./Document.h"
#include "./UiKit.h"
#include "./HomePage.h"
#include "./Navigator.h"
#include "./InstancePage.h"
#include "./SchemaPage.h"
#include "./SearchPage.h"
#include "./SettingsPage.h"

#include "UI/UIEvent.h"
#include "UI/UIWidget.h"

#include <functional>
#include <memory>

namespace eokas::RoseAxion {

    class RoseEditor : public UIWidget
    {
    public:
        struct Host : RoseUiHost
        {
            std::function<void(int)> confirmDeleteSchema;
            std::function<void(int)> confirmDeleteInstance;
            std::function<void(const String&)> openExternal;
            std::function<String(const String& suggested)> pickFilePath;
            std::function<String(const String& suggested)> pickFolderPath;
            std::function<bool()> ctrlDown;
        };

        void bind(TableDocument& doc, const Host& host);
        void reset();
        void resetOutliner();
        void resetSchema();
        void rebuild();
        bool resizeCursorAt(float x, float y) const;
        bool handleEscape();
        bool colorDropping() const;
        void sampleColorDropper(float x, float y, bool commit);
        void cancelColorDropper();
        UIWidget* focusWidget() const;

        void layout() override;

    private:
        void ensurePages();
        void setPage(EditorPage page);
        UIWidget* activePage() const;
        RoseNavigator::Host navigatorHost();
        RoseHomePage::Host homeHost();
        RoseInstancePage::Host instanceHost();
        RoseSchemaPage::Host schemaHost();
        RoseSearchPage::Host searchHost() const;
        RoseSettingsPage::Host settingsHost() const;
        void placeChildren();

        TableDocument* mDoc = nullptr;
        Host mHost;
        EditorPage mPageKind = EditorPage::Instance;
        std::shared_ptr<RoseNavigator> mNavigator;
        std::shared_ptr<RoseHomePage> mHomePage;
        std::shared_ptr<RoseInstancePage> mInstancePage;
        std::shared_ptr<RoseSchemaPage> mSchemaPage;
        std::shared_ptr<RoseSearchPage> mSearchPage;
        std::shared_ptr<RoseSettingsPage> mSettingsPage;
        UIWidget* mOverlay = nullptr;
        float mPaneOutlinerW = kRoseOutlinerDefaultW;
    };

}

#endif
