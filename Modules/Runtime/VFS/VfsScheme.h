#ifndef _EOKAS_VFS_SCHEME_H_
#define _EOKAS_VFS_SCHEME_H_

#include "VfsFile.h"
#include "VfsManifest.h"
#include <memory>
#include <vector>

namespace eokas
{
    class VfsVolume;

    class VfsSchemeFile
    {
    public:
        virtual ~VfsSchemeFile() = default;

        virtual VfsResult read(void* data, u64_t size, u64_t& count) = 0;
        virtual VfsResult write(const void* data, u64_t size, u64_t& count) = 0;
        virtual VfsResult seek(i64_t offset, VfsSeek origin) = 0;
        virtual VfsResult flush() = 0;
        virtual VfsResult close() = 0;
        virtual u64_t tell() const = 0;
        virtual u64_t size() const = 0;
        virtual bool isOpen() const = 0;
        virtual VfsMode mode() const = 0;
    };

    class VfsScheme : public std::enable_shared_from_this<VfsScheme>
    {
    public:
        virtual ~VfsScheme() = default;

        virtual VfsResult stat(const String& path, VfsKind& kind, u64_t& size, i64_t& mtime) = 0;
        virtual VfsResult exists(const String& path, bool& out) = 0;
        virtual VfsResult list(const String& path, std::vector<VfsDirEntry>& out) = 0;
        virtual VfsResult createDir(const String& path) = 0;
        virtual VfsResult remove(const String& path) = 0;
        virtual VfsResult rename(const String& from, const String& to) = 0;
        virtual VfsResult link(const String& existing, const String& path) = 0;
        virtual VfsResult openFile(const String& path, VfsMode mode, VfsFile& file) = 0;
        virtual VfsResult open(std::shared_ptr<VfsVolume> volume, bool writable) = 0;

    protected:
        static VfsResult bind(VfsFile& file, std::unique_ptr<VfsSchemeFile> body);
    };
}

#endif//_EOKAS_VFS_SCHEME_H_
