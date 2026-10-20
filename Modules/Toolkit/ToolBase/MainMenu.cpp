#include "./MainMenu.h"

namespace eokas {

    namespace {

        const Color kMenuBar(0.078f, 0.086f, 0.110f, 1.0f);
        const Color kMenuHover(0.196f, 0.275f, 0.412f, 1.0f);
        const Color kInk(0.925f, 0.933f, 0.953f, 1.0f);
        const Color kClear(0.0f, 0.0f, 0.0f, 0.0f);

    }

    MainMenu::MainMenu() {
        shape.pivot = Vector2(0.0f, 0.0f);
        pickable = false;
        direction = UIDirection::Horizontal;
        padding = 4.0f;
        spacing = 2.0f;
        color = kMenuBar;

        mFile = this->addEntry("File");
        mEdit = this->addEntry("Edit");
        mWindow = this->addEntry("Window");
        mHelp = this->addEntry("Help");
    }

    UIMenuItem* MainMenu::file() const {
        return mFile.get();
    }

    UIMenuItem* MainMenu::edit() const {
        return mEdit.get();
    }

    UIMenuItem* MainMenu::window() const {
        return mWindow.get();
    }

    UIMenuItem* MainMenu::help() const {
        return mHelp.get();
    }

    std::shared_ptr<UIMenuItem> MainMenu::addEntry(const char* title) {
        auto item = std::make_shared<UIMenuItem>();
        item->paddingX = 14.0f;
        item->paddingY = 6.0f;
        item->background = kClear;
        item->hoverFill = kMenuHover;
        item->setText(title);
        if (UIText* label = item->label()) {
            label->style.fontPath = kFontPath;
            label->style.fontSize = kFontSize;
            label->style.color = kInk;
        }
        this->addItem(item);
        return item;
    }

}
