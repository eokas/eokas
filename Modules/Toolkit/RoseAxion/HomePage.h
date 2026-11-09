#ifndef _EOKAS_ROSE_AXION_HOME_PAGE_H_
#define _EOKAS_ROSE_AXION_HOME_PAGE_H_

#include "./Document.h"
#include "./Navigator.h"

#include "UI/UIWidget.h"

#include <functional>

namespace eokas {
    class UIView;
}

namespace eokas::RoseAxion {

    class RoseHomePage : public UIWidget
    {
    public:
        struct Host
        {
            std::function<void()> requestRebuild;
            std::function<void(EditorPage)> selectPage;
            String font;
            float scale = 1.0f;
        };

        void bind(TableDocument& doc, const Host& host);
        void reset();
        void rebuild();
        void layout() override;

    private:
        void notifyRebuild();
        void goTo(EditorPage page);
        void openTable(u32_t index);
        void openSchema(u32_t index);
        void addTable();
        const String& font() const;
        float scale() const;
        String documentTitle() const;
        String documentSubtitle() const;

        TableDocument* mDoc = nullptr;
        Host mHost;
        UIWidget* mBackdrop = nullptr;
        UIView* mView = nullptr;
        float mKeepY = 0.0f;
    };

}

#endif
