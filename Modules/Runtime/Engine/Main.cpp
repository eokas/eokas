#include "./Main.h"

namespace eokas {

    bool Engine::init() {
        return mRenderer.create();
    }

    void Engine::quit() {
        mRenderer.destroy();
    }

    void Engine::tick(f32_t deltaTime) {
        (void)deltaTime;
    }

}
