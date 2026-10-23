#include "VfsManifest.h"
#include "VfsHash.h"
#include <cstring>

namespace eokas
{
    static const u32_t kManifestMagic = 0x464E4D45u;
    static const u16_t kManifestVersion = 2;
    static const int kHashChars = 64;

    static void appendU16(std::vector<u8_t>& out, u16_t value)
    {
        out.push_back((u8_t) (value & 0xff));
        out.push_back((u8_t) ((value >> 8) & 0xff));
    }

    static void appendU32(std::vector<u8_t>& out, u32_t value)
    {
        out.push_back((u8_t) (value & 0xff));
        out.push_back((u8_t) ((value >> 8) & 0xff));
        out.push_back((u8_t) ((value >> 16) & 0xff));
        out.push_back((u8_t) ((value >> 24) & 0xff));
    }

    static void appendU64(std::vector<u8_t>& out, u64_t value)
    {
        for (int shift = 0; shift < 64; shift += 8)
            out.push_back((u8_t) ((value >> shift) & 0xff));
    }

    static void appendI64(std::vector<u8_t>& out, i64_t value)
    {
        appendU64(out, (u64_t) value);
    }

    static void appendHash(std::vector<u8_t>& out, const String& hash)
    {
        for (int index = 0; index < kHashChars; ++index)
        {
            char chr = index < (int) hash.length() ? hash.at((size_t) index) : '\0';
            out.push_back((u8_t) chr);
        }
    }

    struct Reader
    {
        const u8_t* data;
        u64_t size;
        u64_t pos;

        bool take(void* dst, u64_t count)
        {
            if (pos + count > size)
                return false;
            if (count > 0)
                std::memcpy(dst, data + pos, (size_t) count);
            pos += count;
            return true;
        }

        bool u16(u16_t& value)
        {
            u8_t bytes[2];
            if (!take(bytes, 2))
                return false;
            value = (u16_t) bytes[0] | ((u16_t) bytes[1] << 8);
            return true;
        }

        bool u32(u32_t& value)
        {
            u8_t bytes[4];
            if (!take(bytes, 4))
                return false;
            value = (u32_t) bytes[0] | ((u32_t) bytes[1] << 8) | ((u32_t) bytes[2] << 16) | ((u32_t) bytes[3] << 24);
            return true;
        }

        bool u64(u64_t& value)
        {
            u8_t bytes[8];
            if (!take(bytes, 8))
                return false;
            value = 0;
            for (int index = 0; index < 8; ++index)
                value |= ((u64_t) bytes[index]) << (index * 8);
            return true;
        }

        bool i64(i64_t& value)
        {
            u64_t bits = 0;
            if (!u64(bits))
                return false;
            value = (i64_t) bits;
            return true;
        }

        bool hash(String& value)
        {
            char text[kHashChars + 1];
            if (!take(text, kHashChars))
                return false;
            text[kHashChars] = '\0';
            if (std::strlen(text) != kHashChars)
                return false;
            value = String(text);
            return true;
        }
    };

    static VfsChunkSpan wholeSpan(VfsDiffKind kind, const VfsManifest& left, const VfsManifest& right)
    {
        VfsChunkSpan span;
        span.kind = kind;
        span.leftIndex = 0;
        span.rightIndex = 0;
        span.leftLength = left.size > 0xFFFFFFFFull ? 0xFFFFFFFFu : (u32_t) left.size;
        span.rightLength = right.size > 0xFFFFFFFFull ? 0xFFFFFFFFu : (u32_t) right.size;
        span.leftHash = left.hash;
        span.rightHash = right.hash;
        return span;
    }

    VfsResult encodeManifest(const VfsManifest& manifest, std::vector<u8_t>& out)
    {
        if (manifest.hash.length() != kHashChars)
            return VfsResult::Corrupt;
        if (manifest.path.length() > 0xFFFFu)
            return VfsResult::InvalidPath;
        out.clear();
        appendU32(out, kManifestMagic);
        appendU16(out, kManifestVersion);
        appendU16(out, (u16_t) manifest.kind);
        appendU16(out, (u16_t) manifest.path.length());
        appendU64(out, manifest.size);
        appendHash(out, manifest.hash);
        appendI64(out, manifest.mtime);
        appendU32(out, manifest.chunkSize);
        appendU32(out, (u32_t) manifest.chunks.size());
        for (auto& chunk : manifest.chunks)
        {
            if (chunk.hash.length() != kHashChars)
                return VfsResult::Corrupt;
            appendHash(out, chunk.hash);
            appendU32(out, chunk.length);
        }
        const char* path = manifest.path.cstr();
        out.insert(out.end(), path, path + manifest.path.length());
        return VfsResult::Ok;
    }

    VfsResult decodeManifest(const void* data, u64_t size, VfsManifest& out)
    {
        Reader reader{static_cast<const u8_t*>(data), size, 0};
        u32_t magic = 0;
        u16_t version = 0;
        u16_t kind = 0;
        u16_t pathLen = 0;
        if (!reader.u32(magic) || magic != kManifestMagic)
            return VfsResult::Corrupt;
        if (!reader.u16(version) || version != kManifestVersion)
            return VfsResult::Corrupt;
        if (!reader.u16(kind) || !reader.u16(pathLen))
            return VfsResult::Corrupt;
        if (kind != (u16_t) VfsKind::File && kind != (u16_t) VfsKind::Dir && kind != (u16_t) VfsKind::Tombstone)
            return VfsResult::Corrupt;
        VfsManifest manifest;
        manifest.kind = (VfsKind) kind;
        if (!reader.u64(manifest.size) || !reader.hash(manifest.hash) || !reader.i64(manifest.mtime))
            return VfsResult::Corrupt;
        if (!reader.u32(manifest.chunkSize))
            return VfsResult::Corrupt;
        u32_t chunkCount = 0;
        if (!reader.u32(chunkCount))
            return VfsResult::Corrupt;
        if (chunkCount > 1000000u)
            return VfsResult::Corrupt;
        manifest.chunks.reserve(chunkCount);
        for (u32_t index = 0; index < chunkCount; ++index)
        {
            VfsChunkRef chunk;
            if (!reader.hash(chunk.hash) || !reader.u32(chunk.length))
                return VfsResult::Corrupt;
            manifest.chunks.push_back(chunk);
        }
        if (reader.pos + pathLen > reader.size)
            return VfsResult::Corrupt;
        if (pathLen == 0)
        {
            manifest.path = String();
        }
        else
        {
            std::vector<char> path((size_t) pathLen + 1, '\0');
            if (!reader.take(path.data(), pathLen))
                return VfsResult::Corrupt;
            manifest.path = String(path.data());
            if (manifest.path.length() != pathLen)
                return VfsResult::Corrupt;
        }
        out = manifest;
        return VfsResult::Ok;
    }

    VfsResult encodeDir(const std::vector<VfsDirEntry>& entries, std::vector<u8_t>& out)
    {
        out.clear();
        appendU32(out, (u32_t) entries.size());
        for (auto& entry : entries)
        {
            if (entry.name.length() > 0xFFFFu || entry.name.contains("/"))
                return VfsResult::InvalidPath;
            appendU16(out, (u16_t) entry.name.length());
            const char* name = entry.name.cstr();
            out.insert(out.end(), name, name + entry.name.length());
            out.push_back((u8_t) entry.kind);
        }
        return VfsResult::Ok;
    }

    VfsResult decodeDir(const void* data, u64_t size, std::vector<VfsDirEntry>& out)
    {
        Reader reader{static_cast<const u8_t*>(data), size, 0};
        u32_t count = 0;
        if (!reader.u32(count))
            return VfsResult::Corrupt;
        if (count > 1000000u)
            return VfsResult::Corrupt;
        std::vector<VfsDirEntry> entries;
        entries.reserve(count);
        for (u32_t index = 0; index < count; ++index)
        {
            u16_t nameLen = 0;
            if (!reader.u16(nameLen))
                return VfsResult::Corrupt;
            std::vector<char> name((size_t) nameLen + 1, '\0');
            if (nameLen > 0 && !reader.take(name.data(), nameLen))
                return VfsResult::Corrupt;
            u8_t kind = 0;
            if (!reader.take(&kind, 1))
                return VfsResult::Corrupt;
            if (kind != (u8_t) VfsKind::File && kind != (u8_t) VfsKind::Dir && kind != (u8_t) VfsKind::Tombstone)
                return VfsResult::Corrupt;
            VfsDirEntry entry;
            entry.name = String(name.data());
            if (entry.name.length() != nameLen)
                return VfsResult::Corrupt;
            entry.kind = (VfsKind) kind;
            entries.push_back(entry);
        }
        out = std::move(entries);
        return VfsResult::Ok;
    }

    VfsDiff diffManifest(const VfsManifest& left, const VfsManifest& right)
    {
        VfsDiff diff;
        diff.sameKind = left.kind == right.kind;
        diff.sameHash = left.hash == right.hash;
        if (!diff.sameKind)
            return diff;
        if (diff.sameHash)
        {
            diff.spans.push_back(wholeSpan(VfsDiffKind::Equal, left, right));
            return diff;
        }
        if (left.chunkSize != right.chunkSize)
        {
            diff.spans.push_back(wholeSpan(VfsDiffKind::Modified, left, right));
            return diff;
        }

        u32_t shared = (u32_t) left.chunks.size();
        if (right.chunks.size() < shared)
            shared = (u32_t) right.chunks.size();
        for (u32_t index = 0; index < shared; ++index)
        {
            const VfsChunkRef& a = left.chunks[index];
            const VfsChunkRef& b = right.chunks[index];
            VfsChunkSpan span;
            span.leftIndex = index;
            span.rightIndex = index;
            span.leftLength = a.length;
            span.rightLength = b.length;
            span.leftHash = a.hash;
            span.rightHash = b.hash;
            span.kind = a.hash == b.hash ? VfsDiffKind::Equal : VfsDiffKind::Modified;
            diff.spans.push_back(span);
        }
        for (u32_t index = shared; index < left.chunks.size(); ++index)
        {
            VfsChunkSpan span;
            span.kind = VfsDiffKind::LeftOnly;
            span.leftIndex = index;
            span.rightIndex = (u32_t) right.chunks.size();
            span.leftLength = left.chunks[index].length;
            span.rightLength = 0;
            span.leftHash = left.chunks[index].hash;
            diff.spans.push_back(span);
        }
        for (u32_t index = shared; index < right.chunks.size(); ++index)
        {
            VfsChunkSpan span;
            span.kind = VfsDiffKind::RightOnly;
            span.leftIndex = (u32_t) left.chunks.size();
            span.rightIndex = index;
            span.leftLength = 0;
            span.rightLength = right.chunks[index].length;
            span.rightHash = right.chunks[index].hash;
            diff.spans.push_back(span);
        }
        return diff;
    }
}
