#include "./module.h"

namespace eokas {
    static const char* SYMBOL_INSTALL_MODULE = "installModule";
    static const char* SYMBOL_UNINSTALL_MODULE = "uninstallModule";
    
    bool ModuleManager::init() {
        return true;
    }
    
    void ModuleManager::quit() {
        this->clearModules();
    }
    
    void ModuleManager::tick(float deltaTime) {
        for(auto iter = mInitModules.begin(); iter != mInitModules.end(); ++iter) {
            mTickModules.push_back(*iter);
        }
        mInitModules.clear();
        
        for(auto iter = mTickModules.begin(); iter != mTickModules.end(); ++iter) {
            Module* module = *iter;
            module->tick(deltaTime);
        }
        
        for(auto iter = mQuitModules.begin(); iter != mQuitModules.end(); ++iter) {
            Module* module = *iter;
            String moduleName = module->name();
            
            mTickModules.remove(module);
            
            ModulePlugin* plugin = this->getPlugin(moduleName);
            if(plugin != nullptr && plugin->uninstall != nullptr) {
                plugin->uninstall(module);
            }
            else {
                _DeletePointer(module);
            }
            
            if(plugin != nullptr) {
                this->unloadPlugin(moduleName);
            }
        }
        mQuitModules.clear();
    }
    
    Module* ModuleManager::getModule(const String& moduleName) {
        auto iter = mModules.find(moduleName);
        if (iter == mModules.end())
            return nullptr;
        return iter->second;
    }
    
    Module* ModuleManager::loadModule(const String& moduleName) {
        auto iter = mModules.find(moduleName);
        if (iter != mModules.end()) {
            return iter->second;
        }
        
        ModulePlugin* plugin = this->loadPlugin(moduleName);
        if(plugin == nullptr) {
            return nullptr;
        }
        
        Module* module = plugin->install();
        
        // init here to ensure return inited module.
        module->init();
        
        mModules.insert(std::make_pair(moduleName, module));
        mInitModules.push_back(module);
        
        return module;
    }
    
    void ModuleManager::unloadModule(const String& moduleName) {
        auto iter = mModules.find(moduleName);
        if (iter != mModules.end()) {
            Module* module = iter->second;
            module->quit();
            mQuitModules.push_back(module);
            mModules.erase(iter);
        }
    }
    
    void ModuleManager::clearModules() {
        for(auto module : mInitModules) {
            module->quit();
            mQuitModules.push_back(module);
        }
        mInitModules.clear();
        
        for(auto module : mTickModules) {
            module->quit();
            mQuitModules.push_back(module);
        }
        mTickModules.clear();
        
        for(auto module : mQuitModules) {
            ModulePlugin* plugin = this->getPlugin(module->name());
            if(plugin != nullptr && plugin->uninstall != nullptr) {
                plugin->uninstall(module);
            }
            else {
                _DeletePointer(module);
            }
        }
        mQuitModules.clear();
        
        this->clearPlugins();
    }
    
    ModulePlugin* ModuleManager::getPlugin(const String& pluginName) {
        auto iter = mPlugins.find(pluginName);
        if (iter != mPlugins.end()) {
            return &iter->second;
        }
        return nullptr;
    }
    
    ModulePlugin* ModuleManager::loadPlugin(const eokas::String& pluginName) {
        auto iter = mPlugins.find(pluginName);
        if (iter != mPlugins.end()) {
            return &iter->second;
        }
        
        Dll* dll = new Dll(pluginName);
        if (!dll->open()) {
            _DeletePointer(dll);
            return nullptr;
        }
        
        InstallModuleFunc installModule = (InstallModuleFunc) dll->getSymbol(SYMBOL_INSTALL_MODULE);
        UninstallModuleFunc uninstallModule = (UninstallModuleFunc) dll->getSymbol(SYMBOL_UNINSTALL_MODULE);
        if (installModule == nullptr || uninstallModule == nullptr) {
            _DeletePointer(dll);
            return nullptr;
        }
        
        ModulePlugin& plugin = mPlugins[pluginName];
        plugin.dll = dll;
        plugin.install = installModule;
        plugin.uninstall = uninstallModule;
        
        return &plugin;
    }
    
    void ModuleManager::unloadPlugin(const String& pluginName) {
        auto iter = mPlugins.find(pluginName);
        if (iter == mPlugins.end())
            return;
        
        ModulePlugin& plugin = iter->second;
        _DeletePointer(plugin.dll);
        
        mPlugins.erase(iter);
    }
    
    void ModuleManager::clearPlugins() {
        for(auto pluginPair : mPlugins) {
            ModulePlugin& plugin = pluginPair.second;
            _DeletePointer(plugin.dll);
        }
        mPlugins.clear();
    }
}
