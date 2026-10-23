#include "Unit.h"

#include <vector>

using namespace eokas;

EOKAS_TEST_CASE(hash) {
    String input = "eokas-test-hash";
    EOKAS_EXPECT(md5(input) == "165f496c17b648d9ca4e1c8face803c5");
    EOKAS_EXPECT(MD5().compute(input) == "165f496c17b648d9ca4e1c8face803c5");
    EOKAS_EXPECT(sha256(input) == "216fd0525ecaffc3b4a48fc5e98e1e69f387f2627c789df2e8b9c5e90df9c09b");
    EOKAS_EXPECT(SHA256().compute(input) == "216fd0525ecaffc3b4a48fc5e98e1e69f387f2627c789df2e8b9c5e90df9c09b");
    SHA256 split;
    split.reset();
    split.update((const u8_t*) "eokas-", 6);
    split.update((const u8_t*) "test-hash", 9);
    EOKAS_EXPECT(split.finish() == sha256(input));
    SHA256 empty;
    empty.reset();
    EOKAS_EXPECT(empty.finish() == sha256(String("")));
    std::vector<u8_t> block(65, 0x11);
    SHA256 parts;
    parts.reset();
    parts.update(block.data(), 64);
    parts.update(block.data() + 64, 1);
    EOKAS_EXPECT(parts.finish() == SHA256().compute(block.data(), block.size()));
    return 0;
}
