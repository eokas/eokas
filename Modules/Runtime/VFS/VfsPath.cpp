#include "VfsPath.h"
#include "VfsHash.h"

namespace eokas
{
    static bool isSeparator(char chr)
    {
        return chr == '/';
    }

    VfsResult VfsPath::normalize(const String& in, String& out)
    {
        if (in.isEmpty() || in.at(0) != '/')
            return VfsResult::InvalidPath;
        if (in.contains("\\") || in.contains(":"))
            return VfsResult::InvalidPath;
        if (in == "/")
        {
            out = "/";
            return VfsResult::Ok;
        }

        String source = in;
        while (source.length() > 1 && source.at(source.length() - 1) == '/')
            source = source.substr(0, source.length() - 1);

        String built;
        size_t cursor = 1;
        while (cursor <= source.length())
        {
            size_t slash = source.find('/', cursor);
            String segment = slash == String::npos ? source.substr(cursor) : source.substr(cursor, slash - cursor);
            if (segment.isEmpty() || segment == "." || segment == "..")
                return VfsResult::InvalidPath;
            built = built + "/" + segment;
            if (slash == String::npos)
                break;
            cursor = slash + 1;
            if (cursor > source.length())
                return VfsResult::InvalidPath;
        }

        out = built;
        return VfsResult::Ok;
    }

    String VfsPath::parent(const String& path)
    {
        if (path == "/" || path.isEmpty())
            return "/";
        size_t slash = path.rfind('/');
        if (slash == String::npos || slash == 0)
            return "/";
        return path.substr(0, slash);
    }

    String VfsPath::name(const String& path)
    {
        if (path == "/" || path.isEmpty())
            return String();
        size_t slash = path.rfind('/');
        if (slash == String::npos)
            return path;
        return path.substr(slash + 1);
    }

    String VfsPath::join(const String& dir, const String& child)
    {
        if (dir == "/")
            return String("/") + child;
        return dir + "/" + child;
    }

    String VfsPath::hash(const String& normalizedPath)
    {
        return vfsHash(normalizedPath.cstr(), normalizedPath.length());
    }
}
