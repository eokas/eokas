#ifndef _EOKAS_ROSE_AXION_ABOUT_DIALOG_H_
#define _EOKAS_ROSE_AXION_ABOUT_DIALOG_H_

#include "./Host.h"

#include "UI/UIEvent.h"
#include "UI/UIWidget.h"

#include <functional>

namespace eokas::RoseAxion {

    class RoseAboutDialog : public UIWidget
    {
    public:
        struct Host
        {
            String font;
            float scale = 1.0f;
            std::function<void()> onClose;
            std::function<void(const String&)> openExternal;
        };

        void bind(const Host& host);
        void rebuild(float overlayW, float overlayH);
        bool handleEscape();
        UIWidget* focusWidget() const;

    private:
        void close();

        Host mHost;
        UIWidget* mFocus = nullptr;
    };

}

#endif
