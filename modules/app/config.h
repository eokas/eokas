#ifndef _EOKAS_APP_CONFIG_H_
#define _EOKAS_APP_CONFIG_H_

#include "./header.h"

namespace eokas {
    struct AppConfig {
        struct ModuleEntry {
            String name;
            bool required = true;
        };

        String title = "eokas";
        u32_t width = 1280;
        u32_t height = 720;
        std::vector<ModuleEntry> modules;

        static bool load(const String& path, AppConfig& config);
    };
}

#endif//_EOKAS_APP_CONFIG_H_
