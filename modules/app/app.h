#ifndef _EOKAS_APP_APP_H_
#define _EOKAS_APP_APP_H_

#include "./header.h"
#include "./module.h"
#include "./config.h"

namespace eokas
{
    class App
    {
    public:
        ~App();

        static String defaultConfigPath();

        bool init();
        bool init(const String& configPath);
        void quit();
        void tick(float deltaTime);

        const AppConfig& config() const;
        ModuleManager& modules();

    private:
        AppConfig mConfig;
        ModuleManager mModules;
    };
}

#endif//_EOKAS_APP_APP_H_
