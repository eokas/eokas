#include "./MainApp.h"

namespace eokas::Editor {

    bool MainApp::init() {
        this->quit();

        if (!mEngine.init()) {
            return false;
        }

        mStage = std::make_shared<Stage>();
        mStage->setRenderer(mEngine.renderer());
        mStage->setTitle("Editor");
        mStage->setSize(1280.0f, 720.0f);
        if (!mStage->isOpen()) {
            this->quit();
            return false;
        }
        return true;
    }

    void MainApp::quit() {
        mStage.reset();
        mEngine.quit();
    }

    void MainApp::tick(f32_t deltaTime) {
        mEngine.tick(deltaTime);
        if (mStage) {
            mStage->tick(deltaTime);
        }
    }

    Engine& MainApp::engine() {
        return mEngine;
    }

    const Engine& MainApp::engine() const {
        return mEngine;
    }

    Stage* MainApp::stage() const {
        return mStage.get();
    }

}

int main(int, char**) {
    eokas::Editor::MainApp mainApp;
    if (!mainApp.init())
        return 1;

    mainApp.quit();
    return 0;
}
