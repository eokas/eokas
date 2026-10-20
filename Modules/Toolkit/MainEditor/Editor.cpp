#include "./Editor.h"

namespace eokas {

    bool Editor::init() {
        this->quit();

        mMainWindow = std::make_shared<MainWindow>();
        mMainWindow->setTitle("MainEditor");
        mMainWindow->setSize(1280.0f, 720.0f);
        return mMainWindow->isOpen();
    }

    void Editor::quit() {
        mMainWindow.reset();
    }

    void Editor::tick(f32_t deltaTime) {
        if (!mMainWindow) {
            return;
        }
        mMainWindow->tick(deltaTime);
    }

    MainWindow* Editor::mainWindow() const {
        return mMainWindow.get();
    }

}
