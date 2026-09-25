#include "Unit.h"
#include "app/app.h"

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
