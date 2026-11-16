#include "./Utilities.h"

#include "Base/Main.h"

#include <exception>
#include <stdexcept>
#include <string>

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