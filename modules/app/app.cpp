#include "app/app.h"

namespace eokas {
    App::~App() {
        this->quit();
    }

    String App::defaultConfigPath() {
        String exeDir = File::basePath(Process::executingPath());
        String relative = File::combinePath(exeDir, "../config/app.json");
        return File::absolutePath(relative);
    }

    bool App::init() {
        return this->init(App::defaultConfigPath());
    }

    bool App::init(const String& configPath) {
        AppConfig loaded;
        if (!AppConfig::load(configPath, loaded))
            return false;
        mConfig = loaded;

        if (!mModules.init())
            return false;

        for (const AppConfig::ModuleEntry& entry : mConfig.modules) {
            if (mModules.loadModule(entry.name) == nullptr && entry.required) {
                this->quit();
                return false;
            }
        }
        return true;
    }

    void App::quit() {
        mModules.quit();
    }

    void App::tick(float deltaTime) {
        mModules.tick(deltaTime);
    }

    const AppConfig& App::config() const {
        return mConfig;
    }

    ModuleManager& App::modules() {
        return mModules;
    }
}
