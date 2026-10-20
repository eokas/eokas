#ifndef _EOKAS_ENGINE_MAIN_H_
#define _EOKAS_ENGINE_MAIN_H_

#include "./Header.h"
#include "Render/Renderer.h"

namespace eokas {

    class Engine {
    public:
        bool init();
        void quit();
        void tick(f32_t deltaTime);

    private:
        Renderer mRenderer;
    };

}

#endif//_EOKAS_ENGINE_MAIN_H_
