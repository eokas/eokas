#ifndef _EOKAS_VFS_VFS_H_
#define _EOKAS_VFS_VFS_H_

#include "VfsFile.h"
#include "VfsManifest.h"
#include <memory>
#include <vector>

namespace eokas
{
    class VfsVolume;
    class VfsChunkStore;

    struct VfsStat
    {
        VfsKind kind;
        u64_t size;
        String hash;
        i64_t mtime;
    };

    class Vfs
    {
    public:
        Vfs();
        ~Vfs();

        Vfs(const Vfs&) = delete;
        Vfs& operator=(const Vfs&) = delete;

        VfsResult attach(const String& path, std::unique_ptr<VfsVolume> volume, bool writable);
        VfsResult detach(const String& path);
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
        VfsResult diff(const String& left, const String& right, VfsDiff& out);
        VfsResult verify(const String& path, std::vector<u32_t>& badChunks);

    private:
        struct Slot;
        std::vector<Slot*> mSlots;

        void clearSlots();
        Slot* findAttach(const String& path) const;
        Slot* matchAttach(const String& path) const;
        VfsResult resolve(const String& path, Slot*& slot, VfsManifest& manifest) const;
        VfsResult ensureDir(Slot& slot, const String& path);
        VfsResult releaseChunks(VfsChunkStore& store, const VfsManifest& manifest);
    };
}

#endif//_EOKAS_VFS_VFS_H_
