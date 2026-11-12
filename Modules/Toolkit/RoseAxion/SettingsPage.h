#ifndef _EOKAS_ROSE_AXION_SETTINGS_PAGE_H_
#define _EOKAS_ROSE_AXION_SETTINGS_PAGE_H_

#include "./Host.h"

#include "UI/UIWidget.h"

namespace eokas::RoseAxion {

    class RoseSettingsPage : public UIWidget
    {
    public:
        struct Host : RoseUiHost
        {
        };

        void bind(const Host& host);
        void rebuild();
        void layout() override;

    private:
        Host mHost;
    };

}

#endif
