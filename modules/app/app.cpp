#include "app/app.h"
#include "native/dialogs.h"

namespace eokas {
    bool App::init() {
        String configJson;
        if(!File::readText("../config/config.json", configJson))
            return false;
        HomNode json = JSON::parse(configJson);
        
        mModules.init();
        return true;
    }
    
    void App::quit() {
        mModules.quit();
    }
    
    void App::tick(float deltaTime) {
        mModules.quit();
    }
    
    ModuleManager& App::modules() {
        return mModules;
    }
}