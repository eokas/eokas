#include "./Utilities.h"

void Utilities::createImage(std::vector<uint8_t>& image, uint32_t textureWidth, uint32_t textureHeight)
{
    const uint32_t rowPitch = textureWidth * 4;
    const uint32_t cellPitch = rowPitch >> 4; // The width of a cell in the checkboard texture.
    const uint32_t cellHeight = textureWidth >> 4; // The height of a cell in the checkerboard texture.
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
            pData[n + 0] = 0x00;    // R
            pData[n + 1] = 0x00;    // G
            pData[n + 2] = 0x00;    // B
            pData[n + 3] = 0xff;    // A
        }
        else
        {
            pData[n + 0] = 0xff;    // R
            pData[n + 1] = 0xff;    // G
            pData[n + 2] = 0xff;    // B
            pData[n + 3] = 0xff;    // A
        }
    }
}

std::string Utilities::readTextFile(const std::string& fileName)
{
    const size_t maxFileSize = 4096;
    
    char buffer[maxFileSize] = { 0 };
    memset(buffer, 0, maxFileSize);
    
    FILE* fd = NULL;
    fopen_s(&fd, fileName.c_str(), "rb");
    if (fd == NULL) {
        throw std::exception("Open shader file failed.");
    }
    
    fseek(fd, 0, SEEK_END);
    size_t size = ftell(fd);
    if (size > maxFileSize) {
        throw std::exception("The shader file is too large.");
    }
    
    fseek(fd, 0, SEEK_SET);
    fread(buffer, size, 1, fd);
    fclose(fd);
    
    return buffer;
}