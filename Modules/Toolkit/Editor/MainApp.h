#ifndef _EOKAS_EDITOR_MAINAPP_H_
#define _EOKAS_EDITOR_MAINAPP_H_

#include "./Header.h"
#include "Engine/Main.h"
#include "Engine/Stage.h"

#include <memory>

namespace eokas::Editor {

    class MainApp {
    public:
        bool init();
        void quit();
        void tick(f32_t deltaTime);

        Engine& engine();
        const Engine& engine() const;
        Stage* stage() const;

    private:
        Engine mEngine;
        std::shared_ptr<Stage> mStage;
    };

}

#endif//_EOKAS_EDITOR_MAINAPP_H_
