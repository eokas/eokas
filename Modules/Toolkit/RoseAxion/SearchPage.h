#ifndef _EOKAS_ROSE_AXION_SEARCH_PAGE_H_
#define _EOKAS_ROSE_AXION_SEARCH_PAGE_H_

#include "Base/String.h"
#include "UI/UIWidget.h"

namespace eokas::RoseAxion {

    class RoseSearchPage : public UIWidget
    {
    public:
        struct Host
        {
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
