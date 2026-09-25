#include "./config.h"
#include <type_traits>

namespace eokas {
    template<typename T>
    static T homValue(HomNode node, const T& fallback) {
        if constexpr (std::is_same_v<T, String>)
            return node.isString() ? node.asString() : fallback;
        else if constexpr (std::is_same_v<T, bool>)
            return node.isBoolean() ? node.asBoolean() : fallback;
        else if constexpr (std::is_arithmetic_v<T>)
            return node.isNumber() ? static_cast<T>(node.asNumber()) : fallback;
        else {
            static_assert(sizeof(T) == 0, "unsupported hom type");
            return fallback;
        }
    }

    template<typename T>
    static T homValue(HomNode node, const String& key, const T& fallback) {
        if (!node.isObject())
            return fallback;
        return homValue<T>(node.get(key), fallback);
    }

    bool AppConfig::load(const String& path, AppConfig& config) {
        config = AppConfig();

        String text;
        if (!File::readText(path, text))
            return false;

        HomNode json = JSON::parse(text);
        if (!json.isObject())
            return false;

        config.title = homValue<String>(json, "title", config.title);
        config.width = homValue<u32_t>(json, "width", config.width);
        config.height = homValue<u32_t>(json, "height", config.height);

        HomNode modules = json.get("modules");
        if (modules.isNull())
            return true;
        if (!modules.isArray())
            return false;

        bool ok = true;
        modules.foreach([&](const HomNode& item) {
            if (!ok)
                return;

            ModuleEntry entry;
            if (item.isString())
                entry.name = homValue<String>(item, "");
            else if (item.isObject()) {
                entry.name = homValue<String>(item, "name", "");
                entry.required = homValue<bool>(item, "required", true);
            }
            else {
                ok = false;
                return;
            }

            if (entry.name.isEmpty()) {
                ok = false;
                return;
            }
            config.modules.push_back(entry);
        });

        if (!ok) {
            config = AppConfig();
            return false;
        }
        return true;
    }
}
