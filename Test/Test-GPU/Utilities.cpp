#include "./Utilities.h"
#include "Base/Main.h"

#include <stdexcept>

using namespace eokas;

static String resolveContentPath(const String& relativePath)
{
    if (File::exists(relativePath))
        return relativePath;

    String cursor = File::basePath(Process::executingPath());
    for (int i = 0; i < 8; ++i)
    {
        if (cursor.isEmpty())
            break;
        String candidate = File::combinePath(cursor, relativePath);
        if (File::exists(candidate))
            return candidate;
        String parent = File::basePath(cursor);
        if (parent == cursor)
            break;
        cursor = parent;
    }
    return relativePath;
}

void Utilities::createImage(std::vector<uint8_t>& image, uint32_t textureWidth, uint32_t textureHeight)
{
    const uint32_t rowPitch = textureWidth * 4;
    const uint32_t cellPitch = rowPitch >> 4;
    const uint32_t cellHeight = textureWidth >> 4;
    const uint32_t textureSize = rowPitch * textureHeight;

    image.resize(textureSize);

    uint8_t* pData = (uint8_t*)image.data();
    for (uint32_t n = 0; n < textureSize; n += 4)
    {
        uint32_t x = n % rowPitch;
        uint32_t y = n / rowPitch;
        uint32_t i = x / cellPitch;
        uint32_t j = y / cellHeight;

        if (i % 2 == j % 2)
        {
            pData[n + 0] = 0x00;
            pData[n + 1] = 0x00;
            pData[n + 2] = 0x00;
            pData[n + 3] = 0xff;
        }
        else
        {
            pData[n + 0] = 0xff;
            pData[n + 1] = 0xff;
            pData[n + 2] = 0xff;
            pData[n + 3] = 0x80;
        }
    }
}

std::string Utilities::readTextFile(const std::string& fileName)
{
    const size_t maxFileSize = 1024 * 256;

    String resolved = resolveContentPath(fileName.c_str());

    FileStream stream(resolved, "rb");
    if (!stream.open())
        throw std::runtime_error("Open shader file failed.");

    size_t size = stream.size();
    if (size > maxFileSize)
        throw std::runtime_error("The shader file is too large.");

    std::string buffer(size, '\0');
    if (size > 0)
        stream.read(&buffer[0], size);
    stream.close();
    return buffer;
}