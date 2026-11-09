#include "./Prefs.h"
#include "./Theme.h"

#include "Base/Hom.h"
#include "Base/IO.h"
#include "Base/Json.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <ShlObj.h>

#pragma comment(lib, "shell32.lib")

namespace eokas::RoseAxion {

    namespace {

        String prefsDir()
        {
            char appdata[MAX_PATH] = {};
            if (FAILED(SHGetFolderPathA(nullptr, CSIDL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, appdata)))
            {
                return String::empty;
            }
            return File::combinePath(String(appdata), String("RoseAxion"));
        }

        String prefsPath()
        {
            String dir = prefsDir();
            if (dir.isEmpty())
            {
                return String::empty;
            }
            return File::combinePath(dir, String("settings.json"));
        }

    }

    void loadPrefs()
    {
        String path = prefsPath();
        String text;
        if (path.isEmpty() || !File::readText(path, text) || text.isEmpty())
        {
            return;
        }
        HomNode root = JSON::parse(text);
        if (!root.isObject())
        {
            return;
        }
        HomNode node = root.get(String("theme"));
        if (node.isString())
        {
            applyRoseThemeId(themeIdFromKey(node.asString()));
        }
    }

    void savePrefs()
    {
        String dir = prefsDir();
        String path = prefsPath();
        if (dir.isEmpty() || path.isEmpty())
        {
            return;
        }
        CreateDirectoryA(dir.cstr(), nullptr);
        String text = String::format("{\"theme\":\"%s\"}", themeKey(themeId()));
        File::writeText(path, text);
    }

}
