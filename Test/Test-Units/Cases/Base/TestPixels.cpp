#include "Unit.h"

using namespace eokas;

EOKAS_TEST_CASE(pixels) {
    EOKAS_EXPECT(_PixelFormatSize(PixelFormat::A8_UNORM) == 1);
    EOKAS_EXPECT(_PixelFormatSize(PixelFormat::R8G8B8A8_UNORM) == 4);
    EOKAS_EXPECT(_PixelFormatSize(PixelFormat::R16G16B16A16_FLOAT) == 8);
    EOKAS_EXPECT(_R8G8B8A8_R(_R8G8B8A8(1, 2, 3, 4)) == 1);
    EOKAS_EXPECT(_R8G8B8A8_A(_R8G8B8A8(1, 2, 3, 4)) == 4);

    Pixelmap image(2, 3, PixelFormat::R8_UNORM);
    EOKAS_EXPECT(image.width() == 2);
    EOKAS_EXPECT(image.height() == 3);
    EOKAS_EXPECT(image.format() == PixelFormat::R8_UNORM);
    EOKAS_EXPECT(image.data() != nullptr);

    u8_t* bytes = (u8_t*)image.data();
    bytes[0] = 9;
    Pixelmap area = image.getArea(0, 0, 1, 1);
    EOKAS_EXPECT(area.width() == 1);
    EOKAS_EXPECT(area.height() == 1);
    EOKAS_EXPECT(((u8_t*)area.data())[0] == 9);
    return 0;
}
