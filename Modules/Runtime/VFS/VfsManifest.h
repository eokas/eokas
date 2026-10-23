#ifndef _EOKAS_VFS_MANIFEST_H_
#define _EOKAS_VFS_MANIFEST_H_

#include "VfsResult.h"
#include <vector>

namespace eokas
{
    enum class VfsKind : u8_t
    {
        File = 1,
        Dir = 2,
        Tombstone = 3
    };

    enum class VfsDiffKind
    {
        Equal,
        LeftOnly,
        RightOnly,
        Modified
    };

    struct VfsChunkRef
    {
        String hash;
        u32_t length;
    };

    struct VfsChunkSpan
    {
        VfsDiffKind kind;
        u32_t leftIndex;
        u32_t rightIndex;
        u32_t leftLength;
        u32_t rightLength;
        String leftHash;
        String rightHash;
    };

    struct VfsDiff
    {
        bool sameKind;
        bool sameHash;
        std::vector<VfsChunkSpan> spans;
    };

    struct VfsDirEntry
    {
        String name;
        VfsKind kind;
    };

    struct VfsManifest
    {
        VfsKind kind = VfsKind::File;
        String path;
        u64_t size = 0;
        String hash;
        i64_t mtime = 0;
        u32_t chunkSize = 0;
        std::vector<VfsChunkRef> chunks;
    };

    VfsResult encodeManifest(const VfsManifest& manifest, std::vector<u8_t>& out);
    VfsResult decodeManifest(const void* data, u64_t size, VfsManifest& out);
    VfsResult encodeDir(const std::vector<VfsDirEntry>& entries, std::vector<u8_t>& out);
    VfsResult decodeDir(const void* data, u64_t size, std::vector<VfsDirEntry>& out);
    VfsDiff diffManifest(const VfsManifest& left, const VfsManifest& right);
}

#endif//_EOKAS_VFS_MANIFEST_H_
