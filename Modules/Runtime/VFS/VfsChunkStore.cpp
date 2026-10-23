#include "VfsChunkStore.h"
#include "VfsHash.h"
#include "VfsPath.h"
#include "VfsVolume.h"

namespace eokas
{
    static String chunkKey(const String& hash)
    {
        return String("c/") + hash.substr(0, 2) + "/" + hash;
    }

    static String refKey(const String& hash)
    {
        return String("k/") + hash;
    }

    static String manifestKey(const String& path)
    {
        return String("m/") + VfsPath::hash(path);
    }

    static String dirKey(const String& path)
    {
        return String("d/") + VfsPath::hash(path);
    }

    static VfsResult readRef(VfsVolume& volume, const String& hash, u32_t& count, bool& present)
    {
        std::vector<u8_t> bytes;
        VfsResult result = volume.get(refKey(hash), bytes);
        if (result == VfsResult::NotFound)
        {
            count = 0;
            present = false;
            return VfsResult::Ok;
        }
        if (result != VfsResult::Ok)
            return result;
        if (bytes.size() != 4)
            return VfsResult::Corrupt;
        count = (u32_t) bytes[0] | ((u32_t) bytes[1] << 8) | ((u32_t) bytes[2] << 16) | ((u32_t) bytes[3] << 24);
        present = true;
        return VfsResult::Ok;
    }

    static VfsResult writeRef(VfsVolume& volume, const String& hash, u32_t count)
    {
        u8_t bytes[4];
        bytes[0] = (u8_t) (count & 0xff);
        bytes[1] = (u8_t) ((count >> 8) & 0xff);
        bytes[2] = (u8_t) ((count >> 16) & 0xff);
        bytes[3] = (u8_t) ((count >> 24) & 0xff);
        return volume.put(refKey(hash), bytes, 4);
    }

    VfsChunkStore::VfsChunkStore(std::shared_ptr<VfsVolume> volume)
        : mVolume(std::move(volume))
    {
    }

    VfsResult VfsChunkStore::putChunk(const String& hash, const void* data, u64_t size)
    {
        if (hash.length() != 64)
            return VfsResult::Corrupt;
        bool exists = false;
        VfsResult result = mVolume->exists(chunkKey(hash), exists);
        if (result != VfsResult::Ok)
            return result;
        if (exists)
            return VfsResult::Ok;
        return mVolume->put(chunkKey(hash), data, size);
    }

    VfsResult VfsChunkStore::getChunk(const String& hash, std::vector<u8_t>& out)
    {
        if (hash.length() != 64)
            return VfsResult::Corrupt;
        return mVolume->get(chunkKey(hash), out);
    }

    VfsResult VfsChunkStore::addRefs(const String& hash, u32_t count)
    {
        if (count == 0)
            return VfsResult::Ok;
        u32_t current = 0;
        bool present = false;
        VfsResult result = readRef(*mVolume, hash, current, present);
        if (result != VfsResult::Ok)
            return result;
        if (current > 0xFFFFFFFFu - count)
            return VfsResult::Io;
        return writeRef(*mVolume, hash, current + count);
    }

    VfsResult VfsChunkStore::releaseRefs(const String& hash, u32_t count)
    {
        if (count == 0)
            return VfsResult::Ok;
        u32_t current = 0;
        bool present = false;
        VfsResult result = readRef(*mVolume, hash, current, present);
        if (result != VfsResult::Ok)
            return result;
        if (!present || current < count)
            return VfsResult::Corrupt;
        u32_t next = current - count;
        if (next == 0)
        {
            VfsResult removed = mVolume->remove(refKey(hash));
            if (removed != VfsResult::Ok && removed != VfsResult::NotFound)
                return removed;
            removed = mVolume->remove(chunkKey(hash));
            if (removed != VfsResult::Ok && removed != VfsResult::NotFound)
                return removed;
            return VfsResult::Ok;
        }
        return writeRef(*mVolume, hash, next);
    }

    VfsResult VfsChunkStore::readManifest(const String& path, VfsManifest& out)
    {
        std::vector<u8_t> bytes;
        VfsResult result = mVolume->get(manifestKey(path), bytes);
        if (result != VfsResult::Ok)
            return result;
        result = decodeManifest(bytes.data(), bytes.size(), out);
        if (result != VfsResult::Ok)
            return result;
        if (out.path != path)
            return VfsResult::Corrupt;
        return VfsResult::Ok;
    }

    VfsResult VfsChunkStore::writeManifest(const VfsManifest& manifest)
    {
        std::vector<u8_t> bytes;
        VfsResult result = encodeManifest(manifest, bytes);
        if (result != VfsResult::Ok)
            return result;
        return mVolume->put(manifestKey(manifest.path), bytes.data(), bytes.size());
    }

    VfsResult VfsChunkStore::removeManifest(const String& path)
    {
        VfsResult result = mVolume->remove(manifestKey(path));
        if (result == VfsResult::NotFound)
            return VfsResult::Ok;
        return result;
    }

    VfsResult VfsChunkStore::readDir(const String& path, std::vector<VfsDirEntry>& out)
    {
        std::vector<u8_t> bytes;
        VfsResult result = mVolume->get(dirKey(path), bytes);
        if (result != VfsResult::Ok)
            return result;
        return decodeDir(bytes.data(), bytes.size(), out);
    }

    VfsResult VfsChunkStore::writeDir(const String& path, const std::vector<VfsDirEntry>& entries)
    {
        std::vector<u8_t> bytes;
        VfsResult result = encodeDir(entries, bytes);
        if (result != VfsResult::Ok)
            return result;
        return mVolume->put(dirKey(path), bytes.data(), bytes.size());
    }

    VfsResult VfsChunkStore::removeDir(const String& path)
    {
        VfsResult result = mVolume->remove(dirKey(path));
        if (result == VfsResult::NotFound)
            return VfsResult::Ok;
        return result;
    }

    VfsResult VfsChunkStore::upsertChild(const String& dir, const String& name, VfsKind kind)
    {
        if (name.isEmpty() || name.contains("/"))
            return VfsResult::InvalidPath;
        std::vector<VfsDirEntry> entries;
        VfsResult result = readDir(dir, entries);
        if (result == VfsResult::NotFound)
            result = VfsResult::Ok;
        if (result != VfsResult::Ok)
            return result;
        bool found = false;
        for (auto& entry : entries)
        {
            if (entry.name == name)
            {
                entry.kind = kind;
                found = true;
                break;
            }
        }
        if (!found)
        {
            VfsDirEntry entry;
            entry.name = name;
            entry.kind = kind;
            entries.push_back(entry);
        }
        return writeDir(dir, entries);
    }

    VfsResult VfsChunkStore::removeChild(const String& dir, const String& name)
    {
        std::vector<VfsDirEntry> entries;
        VfsResult result = readDir(dir, entries);
        if (result == VfsResult::NotFound)
            return VfsResult::Ok;
        if (result != VfsResult::Ok)
            return result;
        std::vector<VfsDirEntry> kept;
        for (auto& entry : entries)
        {
            if (entry.name != name)
                kept.push_back(entry);
        }
        return writeDir(dir, kept);
    }

    VfsResult VfsChunkStore::verify(const VfsManifest& manifest, std::vector<u32_t>& badChunks)
    {
        badChunks.clear();
        std::vector<u8_t> whole;
        whole.reserve((size_t) manifest.size);
        for (u32_t index = 0; index < manifest.chunks.size(); ++index)
        {
            const VfsChunkRef& chunk = manifest.chunks[index];
            std::vector<u8_t> bytes;
            VfsResult result = getChunk(chunk.hash, bytes);
            bool bad = result != VfsResult::Ok || bytes.size() != chunk.length;
            if (!bad)
            {
                String hash = vfsHash(bytes.data(), bytes.size());
                bad = hash != chunk.hash;
            }
            if (bad)
            {
                badChunks.push_back(index);
                continue;
            }
            whole.insert(whole.end(), bytes.begin(), bytes.end());
        }
        if (!badChunks.empty())
            return VfsResult::Mismatch;
        if (whole.size() != manifest.size)
            return VfsResult::Mismatch;
        String hash = vfsHash(whole.empty() ? nullptr : whole.data(), whole.size());
        if (hash != manifest.hash)
            return VfsResult::Mismatch;
        return VfsResult::Ok;
    }
}
