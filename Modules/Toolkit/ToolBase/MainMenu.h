#ifndef _EOKAS_TOOL_BASE_MAIN_MENU_H_
#define _EOKAS_TOOL_BASE_MAIN_MENU_H_

#include "UI/widgets/UIMenu.h"

namespace eokas {

    class MainMenu : public UIMenu {
    public:
        static constexpr const char* kFontPath = "Content/Fonts/RobotoRegular.ttf";
        static constexpr float kFontSize = 15.0f;

        MainMenu();

        UIMenuItem* file() const;
        UIMenuItem* edit() const;
        UIMenuItem* window() const;
        UIMenuItem* help() const;

    private:
        std::shared_ptr<UIMenuItem> addEntry(const char* title);

        std::shared_ptr<UIMenuItem> mFile;
        std::shared_ptr<UIMenuItem> mEdit;
        std::shared_ptr<UIMenuItem> mWindow;
        std::shared_ptr<UIMenuItem> mHelp;
    };

}

#endif//_EOKAS_TOOL_BASE_MAIN_MENU_H_
