#include "Unit.h"
#include "fbx/FBXImporter.h"

using namespace eokas;

EOKAS_TEST_CASE(importer) {
    FBXImporter importer;
    EOKAS_EXPECT(importer.init());
    importer.quit();
    return 0;
}
