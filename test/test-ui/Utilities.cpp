#include "./Utilities.h"
#include <cstdio>
#include <cstring>
#include <exception>

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
