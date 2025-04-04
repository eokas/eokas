#ifndef _EOKAS_APP_MODULE_H_
#define _EOKAS_APP_MODULE_H_

#include "./header.h"

namespace eokas {
    
    class Module {
    public:
        Module() = default;
        virtual ~Module() = default;
        
        virtual const String& name() const = 0;
        
        virtual bool init() = 0;
        virtual void quit() = 0;
        virtual void tick(float deltaTime) = 0;
    };
    
    using InstallModuleFunc = Module* (*)();
    using UninstallModuleFunc = void(*)(Module*);
    
    struct ModulePlugin {
        Dll* dll = nullptr;
        InstallModuleFunc install = nullptr;
        UninstallModuleFunc uninstall = nullptr;
    };
    
    class ModuleManager {
    public:
        bool init();
        void quit();
        void tick(float deltaTime);
        
        Module* getModule(const String& moduleName);
        Module* loadModule(const String& moduleName);
        void unloadModule(const String& moduleName);
        void clearModules();
    
    private:
        std::map<String, Module*> mModules = {};
        std::list<Module*> mInitModules = {};
        std::list<Module*> mTickModules = {};
        std::list<Module*> mQuitModules = {};
        
        std::map<String, ModulePlugin> mPlugins = {};
        
        ModulePlugin* getPlugin(const String& pluginName);
        ModulePlugin* loadPlugin(const String& pluginName);
        void unloadPlugin(const String& pluginName);
        void clearPlugins();
    };
}

#endif//_EOKAS_APP_MODULE_H_