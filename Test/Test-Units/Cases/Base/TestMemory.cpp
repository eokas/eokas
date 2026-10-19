#include "Unit.h"

using namespace eokas;

EOKAS_TEST_CASE(memory) {
    void* block = MemoryUtility::alloc(8);
    EOKAS_EXPECT(block != nullptr);
    MemoryUtility::clear(block, 8, 0xAB);
    EOKAS_EXPECT(((u8_t*)block)[0] == 0xAB);
    u8_t copy[8] = {};
    MemoryUtility::copy(copy, block, 8);
    EOKAS_EXPECT(MemoryUtility::compare(copy, block, 8) == 0);
    MemoryUtility::free(block);

    MemoryBuffer buffer(4);
    EOKAS_EXPECT(buffer.size() == 4);
    u8_t bytes[4] = {1, 2, 3, 4};
    buffer.fill(bytes, 4);
    EOKAS_EXPECT(((u8_t*)buffer.data())[3] == 4);

    MemoryStream memory;
    EOKAS_EXPECT(memory.open());
    BinaryStream stream(memory);
    EOKAS_EXPECT(stream.write<i32_t>(42));
    EOKAS_EXPECT(stream.write<String>(String("eokas")));
    EOKAS_EXPECT(stream.seek(0, 0));
    i32_t number = 0;
    String text;
    EOKAS_EXPECT(stream.read(number));
    EOKAS_EXPECT(number == 42);
    EOKAS_EXPECT(stream.read(text));
    EOKAS_EXPECT(text == "eokas");

    MemoryStream textMemory;
    textMemory.open();
    TextStream textStream(textMemory);
    EOKAS_EXPECT(textStream.writeLine("line"));
    EOKAS_EXPECT(textStream.seek(0, 0));
    String line;
    EOKAS_EXPECT(textStream.readLine(line));
    EOKAS_EXPECT(line == "line");
    return 0;
}
