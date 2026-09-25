#include "Unit.h"

using namespace eokas;

EOKAS_TEST_CASE(hash) {
    String input = "eokas-test-hash";
    EOKAS_EXPECT(md5(input) == "165f496c17b648d9ca4e1c8face803c5");
    EOKAS_EXPECT(MD5().compute(input) == "165f496c17b648d9ca4e1c8face803c5");
    EOKAS_EXPECT(sha256(input) == "216fd0525ecaffc3b4a48fc5e98e1e69f387f2627c789df2e8b9c5e90df9c09b");
    EOKAS_EXPECT(SHA256().compute(input) == "216fd0525ecaffc3b4a48fc5e98e1e69f387f2627c789df2e8b9c5e90df9c09b");
    return 0;
}
