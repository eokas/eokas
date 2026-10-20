#ifndef _EOKAS_MAIN_EDITOR_EDITOR_H_
#define _EOKAS_MAIN_EDITOR_EDITOR_H_

#include "./Header.h"
#include "ToolBase/MainWindow.h"

#include <memory>

namespace eokas {

    class Editor {
    public:
        bool init();
        void quit();
        void tick(f32_t deltaTime);

        MainWindow* mainWindow() const;

    private:
        std::shared_ptr<MainWindow> mMainWindow;
    };

}

#endif//_EOKAS_MAIN_EDITOR_EDITOR_H_
