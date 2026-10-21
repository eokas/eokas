#include "./MainApp.h"

namespace eokas::MainEditor {

    bool MainApp::init() {
        this->quit();

        if (!mEngine.init()) {
            return false;
        }

        mMainWindow = std::make_shared<MainWindow>();
        mMainWindow->setRenderer(mEngine.renderer());
        mMainWindow->setTitle("MainEditor");
        mMainWindow->setSize(1280.0f, 720.0f);
        if (!mMainWindow->isOpen()) {
            this->quit();
            return false;
        }
        return true;
    }

    void MainApp::quit() {
        mMainWindow.reset();
        mEngine.quit();
    }

    void MainApp::tick(f32_t deltaTime) {
        mEngine.tick(deltaTime);
        if (mMainWindow) {
            mMainWindow->tick(deltaTime);
        }
    }

    Engine& MainApp::engine() {
        return mEngine;
    }

    const Engine& MainApp::engine() const {
        return mEngine;
    }

    MainWindow* MainApp::mainWindow() const {
        return mMainWindow.get();
    }

}

int main(int, char**) {
    eokas::MainEditor::MainApp mainApp;
    if (!mainApp.init())
        return 1;

    mainApp.quit();
    return 0;
}
