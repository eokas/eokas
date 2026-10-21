#ifndef _EOKAS_MAIN_EDITOR_MAINAPP_H_
#define _EOKAS_MAIN_EDITOR_MAINAPP_H_

#include "./Header.h"
#include "Engine/Main.h"
#include "Engine/MainWindow.h"

#include <memory>

namespace eokas::MainEditor {

    class MainApp {
    public:
        bool init();
        void quit();
        void tick(f32_t deltaTime);

        Engine& engine();
        const Engine& engine() const;
        MainWindow* mainWindow() const;

    private:
        Engine mEngine;
        std::shared_ptr<MainWindow> mMainWindow;
    };

}

#endif//_EOKAS_MAIN_EDITOR_MAINAPP_H_
