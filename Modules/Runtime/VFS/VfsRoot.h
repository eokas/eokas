#ifndef _EOKAS_VFS_ROOT_H_
#define _EOKAS_VFS_ROOT_H_

#include "VfsFile.h"
#include "VfsManifest.h"
#include <memory>
#include <vector>

namespace eokas
{
    class VfsVolume;
    class VfsScheme;

    struct VfsStat
    {
        VfsKind kind;
        u64_t size;
        i64_t mtime;
    };

    class VfsRoot
    {
    public:
        VfsRoot();
        ~VfsRoot();

        VfsRoot(const VfsRoot&) = delete;
        VfsRoot& operator=(const VfsRoot&) = delete;

        VfsResult mount(const String& path, std::unique_ptr<VfsVolume> volume);
        VfsResult unmount(const String& path);
        VfsResult exists(const String& path, bool& out);
        VfsResult stat(const String& path, VfsStat& out);
        VfsResult list(const String& path, std::vector<VfsDirEntry>& out);
        VfsResult createDir(const String& path);
        VfsResult remove(const String& path);
        VfsResult open(const String& path, VfsMode mode, VfsFile& file);
        VfsResult readAll(const String& path, std::vector<u8_t>& out);
        VfsResult writeAll(const String& path, const void* data, u64_t size);
        VfsResult readText(const String& path, String& text);
        VfsResult writeText(const String& path, const String& text);
        VfsResult rename(const String& from, const String& to);
        VfsResult link(const String& existing, const String& path);

    private:
        struct Mount;
        std::vector<Mount*> mMounts;

        void clearMounts();
        Mount* findMount(const String& path) const;
        Mount* matchMount(const String& path) const;
    };
}

#endif//_EOKAS_VFS_ROOT_H_
