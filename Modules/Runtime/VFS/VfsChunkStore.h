#ifndef _EOKAS_VFS_CHUNK_STORE_H_
#define _EOKAS_VFS_CHUNK_STORE_H_

#include "VfsManifest.h"
#include <memory>

namespace eokas
{
    class VfsVolume;

    class VfsChunkStore
    {
    public:
        explicit VfsChunkStore(std::shared_ptr<VfsVolume> volume);

        VfsResult putChunk(const String& hash, const void* data, u64_t size);
        VfsResult getChunk(const String& hash, std::vector<u8_t>& out);
        VfsResult addRefs(const String& hash, u32_t count);
        VfsResult releaseRefs(const String& hash, u32_t count);

        VfsResult readManifest(const String& path, VfsManifest& out);
        VfsResult writeManifest(const VfsManifest& manifest);
        VfsResult removeManifest(const String& path);

        VfsResult readDir(const String& path, std::vector<VfsDirEntry>& out);
        VfsResult writeDir(const String& path, const std::vector<VfsDirEntry>& entries);
        VfsResult removeDir(const String& path);
        VfsResult upsertChild(const String& dir, const String& name, VfsKind kind);
        VfsResult removeChild(const String& dir, const String& name);

        VfsResult verify(const VfsManifest& manifest, std::vector<u32_t>& badChunks);

    private:
        std::shared_ptr<VfsVolume> mVolume;
    };
}

#endif//_EOKAS_VFS_CHUNK_STORE_H_
