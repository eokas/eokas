#ifndef _EOKAS_ROSE_AXION_SETTINGS_PAGE_H_
#define _EOKAS_ROSE_AXION_SETTINGS_PAGE_H_

#include "Base/String.h"
#include "UI/UIWidget.h"

#include <functional>

namespace eokas::RoseAxion {

    class RoseSettingsPage : public UIWidget
    {
    public:
        struct Host
        {
            std::function<void()> requestRebuild;
            String font;
            float scale = 1.0f;
        };

        void bind(const Host& host);
        void rebuild();
        void layout() override;

    private:
        Host mHost;
    };

}

#endif
