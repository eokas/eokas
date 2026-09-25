#include "Unit.h"
#include "imapp/app/app.h"

EOKAS_TEST_CASE(imapp) {
    ImApp app;
    app.init();
    app.quit();
    return 0;
}
