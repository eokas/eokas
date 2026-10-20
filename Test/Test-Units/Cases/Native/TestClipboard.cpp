#include "Unit.h"
#include "Native/NativeClipboard.h"

#include <cstring>

using namespace eokas;

EOKAS_TEST_CASE(clipboard) {
    const char utf8[] = "eokas-\xE5\x89\xAA\xE8\xB4\xB4\xE6\x9D\xBF";
    const String text(utf8);
    EOKAS_EXPECT(NativeClipboard::writeText(text));
    String read;
    EOKAS_EXPECT(NativeClipboard::readText(read));
    EOKAS_EXPECT(read == text);

    const unsigned char bytes[] = { 0x00, 0x01, 0x7F, 0x80, 0xFF, 0x10, 0x20, 0x30 };
    EOKAS_EXPECT(NativeClipboard::writeBinary(bytes, sizeof(bytes)));
    unsigned char small[4] = {};
    size_t size = 0;
    EOKAS_EXPECT(!NativeClipboard::readBinary(small, sizeof(small), size));
    EOKAS_EXPECT(size == sizeof(bytes));
    unsigned char actual[sizeof(bytes)] = {};
    EOKAS_EXPECT(NativeClipboard::readBinary(actual, sizeof(actual), size));
    EOKAS_EXPECT(size == sizeof(bytes));
    EOKAS_EXPECT(std::memcmp(actual, bytes, sizeof(bytes)) == 0);
    return 0;
}
