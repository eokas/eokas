#pragma once
#include <vector>
#include <string>

struct Utilities
{
    static void createImage(std::vector<uint8_t>& image, uint32_t textureWidth, uint32_t textureHeight);
    static std::string readTextFile(const std::string& fileName);
};
