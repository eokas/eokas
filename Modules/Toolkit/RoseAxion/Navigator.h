#ifndef _EOKAS_ROSE_AXION_NAVIGATOR_H_
#define _EOKAS_ROSE_AXION_NAVIGATOR_H_

#include "./Document.h"
#include "./Host.h"

#include "UI/UIEvent.h"
#include "UI/UIWidget.h"

#include <functional>
#include <memory>

namespace eokas::RoseAxion {

    enum class EditorPage
    {
        Home,
        Instance,
        Schemas,
        Search,
        Settings
    };

    class RoseNavigator : public UIWidget
    {
    public:
        struct Host : RoseUiHost
        {
            std::function<void(EditorPage)> selectPage;
            EditorPage current = EditorPage::Instance;
        };

        void bind(const Host& host);
        void rebuild();
        float barWidth() const;
        void layout() override;

    private:
        const String& font() const;
        float scale() const;

        Host mHost;
        UIWidget* mHome = nullptr;
        UIWidget* mInstance = nullptr;
        UIWidget* mSchemas = nullptr;
        UIWidget* mSearch = nullptr;
        UIWidget* mSettings = nullptr;
    };

}

#endif
