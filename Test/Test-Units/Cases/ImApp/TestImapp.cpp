#include "Unit.h"
#include "ImApp/app/App.h"

EOKAS_TEST_CASE(imapp) {
    ImApp app;
    app.init();
    app.quit();
    return 0;
}
