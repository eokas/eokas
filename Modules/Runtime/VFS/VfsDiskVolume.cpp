#include "VfsDiskVolume.h"
#include "Base/IO.h"
#include <cstdio>

#if _EOKAS_OS == _EOKAS_OS_WIN64 || _EOKAS_OS == _EOKAS_OS_WIN32
#include <direct.h>
#include <io.h>
#include <windows.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace eokas
{
    static bool validKey(const String& key)
    {
        if (key.isEmpty() || key.contains("..") || key.contains("\\"))
            return false;
        if (key.at(0) == '/' || key.at(key.length() - 1) == '/')
            return false;
        size_t cursor = 0;
        while (cursor <= key.length())
        {
            size_t slash = key.find('/', cursor);
            String segment = slash == String::npos ? key.substr(cursor) : key.substr(cursor, slash - cursor);
            if (segment.isEmpty())
                return false;
            for (size_t index = 0; index < segment.length(); ++index)
            {
                char chr = segment.at(index);
                bool digit = chr >= '0' && chr <= '9';
                bool lower = chr >= 'a' && chr <= 'z';
                if (!digit && !lower)
                    return false;
            }
            if (slash == String::npos)
                break;
            cursor = slash + 1;
        }
        return true;
    }

    static void makeDirectory(const String& path)
    {
#if _EOKAS_OS == _EOKAS_OS_WIN64 || _EOKAS_OS == _EOKAS_OS_WIN32
        _mkdir(path.cstr());
#else
        mkdir(path.cstr(), 0755);
#endif
    }

    static void ensureParent(const String& filePath)
    {
        size_t cursor = 0;
        while (cursor < filePath.length())
        {
            size_t slash = filePath.find('/', cursor);
            if (slash == String::npos)
                break;
            if (slash > 0)
                makeDirectory(filePath.substr(0, slash));
            cursor = slash + 1;
        }
    }

    static bool isDriveRoot(const String& path)
    {
        if (path.length() != 3 || path.at(1) != ':' || path.at(2) != '/')
            return false;
        char drive = path.at(0);
        bool upper = drive >= 'A' && drive <= 'Z';
        bool lower = drive >= 'a' && drive <= 'z';
        return upper || lower;
    }

    static String hostRoot(const String& root)
    {
        String normalized;
        for (size_t index = 0; index < root.length(); ++index)
        {
            char chr = root.at(index);
            if (chr == '\\')
                chr = '/';
            normalized += String(chr);
        }
        while (normalized.length() > 1 && normalized.at(normalized.length() - 1) == '/' && !isDriveRoot(normalized))
            normalized = normalized.substr(0, normalized.length() - 1);
        return normalized;
    }

    static bool readSize(FILE* handle, u64_t& out)
    {
#if _EOKAS_OS == _EOKAS_OS_WIN64 || _EOKAS_OS == _EOKAS_OS_WIN32
        if (_fseeki64(handle, 0, SEEK_END) != 0)
            return false;
        __int64 length = _ftelli64(handle);
        if (length < 0 || _fseeki64(handle, 0, SEEK_SET) != 0)
            return false;
#else
        if (fseeko(handle, 0, SEEK_END) != 0)
            return false;
        off_t length = ftello(handle);
        if (length < 0 || fseeko(handle, 0, SEEK_SET) != 0)
            return false;
#endif
        out = (u64_t) length;
        return true;
    }

    static bool commitFile(FILE* handle)
    {
        if (std::fflush(handle) != 0)
            return false;
#if _EOKAS_OS == _EOKAS_OS_WIN64 || _EOKAS_OS == _EOKAS_OS_WIN32
        return _commit(_fileno(handle)) == 0;
#else
        return fsync(fileno(handle)) == 0;
#endif
    }

    static bool replaceFile(const String& temp, const String& path)
    {
#if _EOKAS_OS == _EOKAS_OS_WIN64 || _EOKAS_OS == _EOKAS_OS_WIN32
        return MoveFileExA(temp.cstr(), path.cstr(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
        return std::rename(temp.cstr(), path.cstr()) == 0;
#endif
    }

    VfsDiskVolume::VfsDiskVolume(const String& root)
        : mRoot(hostRoot(root))
    {
        if (!mRoot.isEmpty())
            ensureParent(mRoot + "/child");
    }

    VfsResult VfsDiskVolume::exists(const String& key, bool& out)
    {
        if (!validKey(key))
            return VfsResult::InvalidPath;
        out = File::isFile(File::combinePath(mRoot, key));
        return VfsResult::Ok;
    }

    VfsResult VfsDiskVolume::get(const String& key, std::vector<u8_t>& out)
    {
        if (!validKey(key))
            return VfsResult::InvalidPath;
        String path = File::combinePath(mRoot, key);
        if (!File::isFile(path))
            return VfsResult::NotFound;
        FILE* handle = std::fopen(path.cstr(), "rb");
        if (handle == nullptr)
            return VfsResult::Io;
        u64_t length = 0;
        if (!readSize(handle, length) || length > (u64_t) static_cast<size_t>(-1))
        {
            std::fclose(handle);
            return VfsResult::Io;
        }
        out.assign((size_t) length, 0);
        if (length > 0)
        {
            size_t got = std::fread(out.data(), 1, (size_t) length, handle);
            std::fclose(handle);
            if (got != (size_t) length)
                return VfsResult::Io;
        }
        else
        {
            std::fclose(handle);
        }
        return VfsResult::Ok;
    }

    VfsResult VfsDiskVolume::put(const String& key, const void* data, u64_t size)
    {
        if (!validKey(key))
            return VfsResult::InvalidPath;
        if (size > (u64_t) static_cast<size_t>(-1))
            return VfsResult::Io;
        String path = File::combinePath(mRoot, key);
        ensureParent(path);
        String temp = path + ".tmp";
        FILE* handle = std::fopen(temp.cstr(), "wb");
        if (handle == nullptr)
            return VfsResult::Io;
        if (size > 0 && data != nullptr)
        {
            size_t wrote = std::fwrite(data, 1, (size_t) size, handle);
            if (wrote != (size_t) size || !commitFile(handle))
            {
                std::fclose(handle);
                std::remove(temp.cstr());
                return VfsResult::Io;
            }
        }
        else if (!commitFile(handle))
        {
            std::fclose(handle);
            std::remove(temp.cstr());
            return VfsResult::Io;
        }
        std::fclose(handle);
        if (!replaceFile(temp, path))
        {
            std::remove(temp.cstr());
            return VfsResult::Io;
        }
        return VfsResult::Ok;
    }

    VfsResult VfsDiskVolume::remove(const String& key)
    {
        if (!validKey(key))
            return VfsResult::InvalidPath;
        String path = File::combinePath(mRoot, key);
        if (!File::isFile(path))
            return VfsResult::NotFound;
        if (std::remove(path.cstr()) != 0)
            return VfsResult::Io;
        return VfsResult::Ok;
    }

    static void walk(const String& directory, const String& prefix, std::vector<String>& out)
    {
        if (!File::isFolder(directory))
            return;
        StringList files = File::listFileNames(directory);
        for (auto& name : files)
        {
            String key = prefix.isEmpty() ? name : prefix + "/" + name;
            out.push_back(key);
        }
        StringList folders = File::listFolderNames(directory);
        for (auto& name : folders)
        {
            String child = prefix.isEmpty() ? name : prefix + "/" + name;
            walk(File::combinePath(directory, name), child, out);
        }
    }

    VfsResult VfsDiskVolume::list(const String& prefix, std::vector<String>& out)
    {
        out.clear();
        std::vector<String> keys;
        walk(mRoot, String(), keys);
        for (auto& key : keys)
        {
            if (key.startsWith(prefix))
                out.push_back(key);
        }
        return VfsResult::Ok;
    }
}
