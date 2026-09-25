#include "Unit.h"
#include "app/app.h"

#include <cstdio>

using namespace eokas;

EOKAS_TEST_CASE(module) {
    ModuleManager manager;
    EOKAS_EXPECT(manager.init());
    EOKAS_EXPECT(manager.getModule("missing") == nullptr);
    EOKAS_EXPECT(manager.loadModule("eokas-unit-missing-plugin") == nullptr);
    manager.tick(0.016f);
    manager.quit();

    App app;
    EOKAS_EXPECT(app.modules().getModule("missing") == nullptr);
    app.tick(0.0f);
    app.quit();
    return 0;
}

EOKAS_TEST_CASE(app) {
    App missing;
    EOKAS_EXPECT(!missing.init("eokas-unit-app-missing.json"));

    String path = "eokas-unit-app.json";
    String text = "{\"title\":\"probe\",\"width\":640,\"height\":480,\"modules\":[{\"name\":\"missing-plugin\",\"required\":false}]}";
    EOKAS_EXPECT(File::writeText(path, text));
    {
        App app;
        EOKAS_EXPECT(app.init(path));
        EOKAS_EXPECT(app.config().title == "probe");
        EOKAS_EXPECT(app.config().width == 640);
        EOKAS_EXPECT(app.config().height == 480);
        EOKAS_EXPECT(app.modules().getModule("missing-plugin") == nullptr);
        app.tick(0.0f);
    }
    std::remove(path.cstr());

    text = "{\"modules\":[{\"name\":\"missing-plugin\",\"required\":true}]}";
    EOKAS_EXPECT(File::writeText(path, text));
    {
        App app;
        EOKAS_EXPECT(!app.init(path));
    }
    std::remove(path.cstr());

    EOKAS_EXPECT(App::defaultConfigPath().endsWith("app.json"));
    return 0;
}
