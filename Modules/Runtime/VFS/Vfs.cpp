#include "Vfs.h"
#include "VfsChunkStore.h"
#include "VfsHash.h"
#include "VfsPath.h"
#include "VfsVolume.h"
#include <ctime>
#include <map>

namespace eokas
{
    struct Vfs::Slot
    {
        String path;
        bool writable = false;
        std::shared_ptr<VfsVolume> volume;
        std::shared_ptr<VfsChunkStore> store;
    };

    static bool covers(const String& attached, const String& path)
    {
        if (attached == "/")
            return true;
        if (path == attached)
            return true;
        String prefix = attached + "/";
        return path.startsWith(prefix);
    }

    static void syntheticDir(const String& path, VfsManifest& manifest)
    {
        manifest.kind = VfsKind::Dir;
        manifest.path = path;
        manifest.size = 0;
        manifest.hash = vfsHash(nullptr, 0);
        manifest.mtime = 0;
        manifest.chunkSize = VfsChunkBytes;
        manifest.chunks.clear();
    }

    Vfs::Vfs()
    {
    }

    Vfs::~Vfs()
    {
        clearSlots();
    }

    void Vfs::clearSlots()
    {
        for (auto* slot : mSlots)
            delete slot;
        mSlots.clear();
    }

    Vfs::Slot* Vfs::findAttach(const String& path) const
    {
        for (auto* slot : mSlots)
        {
            if (slot->path == path)
                return slot;
        }
        return nullptr;
    }

    Vfs::Slot* Vfs::matchAttach(const String& path) const
    {
        Slot* best = nullptr;
        for (auto* slot : mSlots)
        {
            if (!covers(slot->path, path))
                continue;
            if (best == nullptr || slot->path.length() > best->path.length())
                best = slot;
        }
        return best;
    }

    VfsResult Vfs::resolve(const String& path, Slot*& slot, VfsManifest& manifest) const
    {
        Slot* match = matchAttach(path);
        slot = match;
        if (match == nullptr)
            return VfsResult::NotFound;
        if (match->path == path)
        {
            VfsResult result = match->store->readManifest(path, manifest);
            if (result == VfsResult::Ok && manifest.kind == VfsKind::Dir)
                return VfsResult::Ok;
            if (result != VfsResult::Ok && result != VfsResult::NotFound)
                return result;
            syntheticDir(path, manifest);
            return VfsResult::Ok;
        }
        VfsResult result = match->store->readManifest(path, manifest);
        if (result != VfsResult::Ok)
            return result;
        if (manifest.kind == VfsKind::Tombstone)
            return VfsResult::NotFound;
        return VfsResult::Ok;
    }

    VfsResult Vfs::ensureDir(Slot& slot, const String& path)
    {
        if (path == slot.path)
            return VfsResult::Ok;
        VfsManifest existing;
        VfsResult result = slot.store->readManifest(path, existing);
        if (result == VfsResult::Ok && existing.kind == VfsKind::Dir)
            return VfsResult::Ok;
        if (result == VfsResult::Ok && existing.kind == VfsKind::File)
            return VfsResult::AlreadyExists;
        if (result != VfsResult::Ok && result != VfsResult::NotFound)
            return result;
        if (path != slot.path)
        {
            VfsResult parent = ensureDir(slot, VfsPath::parent(path));
            if (parent != VfsResult::Ok)
                return parent;
        }
        VfsManifest dir;
        dir.kind = VfsKind::Dir;
        dir.path = path;
        dir.size = 0;
        dir.hash = vfsHash(nullptr, 0);
        dir.mtime = (i64_t) std::time(nullptr);
        dir.chunkSize = VfsChunkBytes;
        result = slot.store->writeManifest(dir);
        if (result != VfsResult::Ok)
            return result;
        if (path == slot.path)
            return VfsResult::Ok;
        return slot.store->upsertChild(VfsPath::parent(path), VfsPath::name(path), VfsKind::Dir);
    }

    VfsResult Vfs::releaseChunks(VfsChunkStore& store, const VfsManifest& manifest)
    {
        std::map<String, u32_t> counts;
        for (auto& chunk : manifest.chunks)
            counts[chunk.hash] += 1;
        for (auto& item : counts)
        {
            VfsResult result = store.releaseRefs(item.first, item.second);
            if (result != VfsResult::Ok)
                return result;
        }
        return VfsResult::Ok;
    }

    VfsResult Vfs::attach(const String& path, std::unique_ptr<VfsVolume> volume, bool writable)
    {
        String normal;
        VfsResult result = VfsPath::normalize(path, normal);
        if (result != VfsResult::Ok)
            return result;
        if (volume == nullptr)
            return VfsResult::Io;
        if (findAttach(normal) != nullptr)
            return VfsResult::AlreadyExists;
        String cursor = normal;
        while (cursor != "/")
        {
            Slot* owner = matchAttach(cursor);
            if (owner != nullptr)
            {
                VfsManifest manifest;
                VfsResult read = owner->store->readManifest(cursor, manifest);
                if (read == VfsResult::Ok && manifest.kind == VfsKind::File)
                    return VfsResult::AlreadyExists;
                if (read != VfsResult::Ok && read != VfsResult::NotFound)
                    return read;
            }
            cursor = VfsPath::parent(cursor);
        }
        Slot* slot = new Slot();
        slot->path = normal;
        slot->writable = writable;
        slot->volume = std::shared_ptr<VfsVolume>(volume.release());
        slot->store = std::make_shared<VfsChunkStore>(slot->volume);
        mSlots.push_back(slot);
        return VfsResult::Ok;
    }

    VfsResult Vfs::detach(const String& path)
    {
        String normal;
        VfsResult result = VfsPath::normalize(path, normal);
        if (result != VfsResult::Ok)
            return result;
        for (auto iter = mSlots.begin(); iter != mSlots.end(); ++iter)
        {
            if ((*iter)->path != normal)
                continue;
            delete *iter;
            mSlots.erase(iter);
            return VfsResult::Ok;
        }
        return VfsResult::NotFound;
    }

    VfsResult Vfs::exists(const String& path, bool& out)
    {
        String normal;
        VfsResult result = VfsPath::normalize(path, normal);
        if (result != VfsResult::Ok)
            return result;
        if (normal == "/")
        {
            out = true;
            return VfsResult::Ok;
        }
        Slot* slot = nullptr;
        VfsManifest manifest;
        result = resolve(normal, slot, manifest);
        if (result == VfsResult::NotFound)
        {
            out = false;
            return VfsResult::Ok;
        }
        if (result != VfsResult::Ok)
            return result;
        out = true;
        return VfsResult::Ok;
    }

    VfsResult Vfs::stat(const String& path, VfsStat& out)
    {
        String normal;
        VfsResult result = VfsPath::normalize(path, normal);
        if (result != VfsResult::Ok)
            return result;
        if (normal == "/")
        {
            out.kind = VfsKind::Dir;
            out.size = 0;
            out.hash = vfsHash(nullptr, 0);
            out.mtime = 0;
            return VfsResult::Ok;
        }
        Slot* slot = nullptr;
        VfsManifest manifest;
        result = resolve(normal, slot, manifest);
        if (result != VfsResult::Ok)
            return result;
        out.kind = manifest.kind;
        out.size = manifest.size;
        out.hash = manifest.hash;
        out.mtime = manifest.mtime;
        return VfsResult::Ok;
    }

    VfsResult Vfs::list(const String& path, std::vector<VfsDirEntry>& out)
    {
        out.clear();
        String normal;
        VfsResult result = VfsPath::normalize(path, normal);
        if (result != VfsResult::Ok)
            return result;
        if (normal != "/")
        {
            Slot* slot = nullptr;
            VfsManifest manifest;
            result = resolve(normal, slot, manifest);
            if (result != VfsResult::Ok)
                return result;
            if (manifest.kind != VfsKind::Dir)
                return VfsResult::Access;
        }
        Slot* slot = matchAttach(normal);
        std::map<String, VfsDirEntry> merged;
        if (slot != nullptr)
        {
            std::vector<VfsDirEntry> entries;
            VfsResult listed = slot->store->readDir(normal, entries);
            if (listed != VfsResult::Ok && listed != VfsResult::NotFound)
                return listed;
            if (listed == VfsResult::Ok)
            {
                for (auto& entry : entries)
                {
                    if (entry.kind != VfsKind::Tombstone)
                        merged[entry.name] = entry;
                }
            }
        }
        for (auto* child : mSlots)
        {
            if (child->path == "/" || VfsPath::parent(child->path) != normal)
                continue;
            VfsDirEntry entry;
            entry.name = VfsPath::name(child->path);
            entry.kind = VfsKind::Dir;
            merged[entry.name] = entry;
        }
        for (auto& item : merged)
            out.push_back(item.second);
        return VfsResult::Ok;
    }

    VfsResult Vfs::createDir(const String& path)
    {
        String normal;
        VfsResult result = VfsPath::normalize(path, normal);
        if (result != VfsResult::Ok)
            return result;
        if (normal == "/")
            return VfsResult::Ok;
        if (findAttach(normal) != nullptr)
            return VfsResult::Ok;
        Slot* writer = matchAttach(normal);
        if (writer == nullptr)
            return VfsResult::NotFound;
        if (!writer->writable)
            return VfsResult::Access;
        VfsManifest manifest;
        result = writer->store->readManifest(normal, manifest);
        if (result == VfsResult::Ok && manifest.kind == VfsKind::Dir)
            return VfsResult::Ok;
        if (result == VfsResult::Ok && manifest.kind == VfsKind::File)
            return VfsResult::AlreadyExists;
        if (result != VfsResult::Ok && result != VfsResult::NotFound)
            return result;
        return ensureDir(*writer, normal);
    }

    VfsResult Vfs::remove(const String& path)
    {
        String normal;
        VfsResult result = VfsPath::normalize(path, normal);
        if (result != VfsResult::Ok)
            return result;
        if (normal == "/")
            return VfsResult::Access;
        if (findAttach(normal) != nullptr)
            return VfsResult::Access;
        Slot* writer = nullptr;
        VfsManifest manifest;
        result = resolve(normal, writer, manifest);
        if (result != VfsResult::Ok)
            return result;
        if (!writer->writable)
            return VfsResult::Access;
        if (manifest.kind == VfsKind::Dir)
        {
            std::vector<VfsDirEntry> children;
            VfsResult listed = list(normal, children);
            if (listed != VfsResult::Ok)
                return listed;
            if (!children.empty())
                return VfsResult::NotEmpty;
        }
        VfsManifest local;
        VfsResult localResult = writer->store->readManifest(normal, local);
        if (localResult != VfsResult::Ok && localResult != VfsResult::NotFound)
            return localResult;
        VfsResult removed = writer->store->removeManifest(normal);
        if (removed != VfsResult::Ok)
            return removed;
        if (localResult == VfsResult::Ok && local.kind == VfsKind::File)
        {
            VfsResult released = releaseChunks(*writer->store, local);
            if (released != VfsResult::Ok)
                return released;
        }
        if (manifest.kind == VfsKind::Dir)
        {
            removed = writer->store->removeDir(normal);
            if (removed != VfsResult::Ok)
                return removed;
        }
        return writer->store->removeChild(VfsPath::parent(normal), VfsPath::name(normal));
    }

    VfsResult Vfs::open(const String& path, VfsMode mode, VfsFile& file)
    {
        String normal;
        VfsResult result = VfsPath::normalize(path, normal);
        if (result != VfsResult::Ok)
            return result;
        if (normal == "/" || findAttach(normal) != nullptr)
            return VfsResult::Access;

        Slot* slot = nullptr;
        VfsManifest manifest;
        result = resolve(normal, slot, manifest);
        bool found = result == VfsResult::Ok;
        if (!found && result != VfsResult::NotFound)
            return result;
        if (slot == nullptr)
            return VfsResult::NotFound;

        if (mode == VfsMode::Read)
        {
            if (!found)
                return VfsResult::NotFound;
            if (manifest.kind != VfsKind::File)
                return VfsResult::Access;
            return file.prepare(slot->store, nullptr, manifest, nullptr, VfsMode::Read, false, false, 0);
        }

        if (!slot->writable)
            return VfsResult::Access;
        if (found && manifest.kind == VfsKind::Dir)
            return VfsResult::AlreadyExists;
        VfsResult ensured = ensureDir(*slot, VfsPath::parent(normal));
        if (ensured != VfsResult::Ok)
            return ensured;

        VfsManifest local;
        VfsResult localResult = slot->store->readManifest(normal, local);
        if (localResult != VfsResult::Ok && localResult != VfsResult::NotFound)
            return localResult;
        bool owns = localResult == VfsResult::Ok && local.kind == VfsKind::File;

        VfsManifest logical;
        logical.kind = VfsKind::File;
        logical.path = normal;
        logical.chunkSize = VfsChunkBytes;
        logical.hash = vfsHash(nullptr, 0);
        const VfsManifest* baseline = nullptr;
        bool materialize = false;
        bool dirty = false;
        u64_t pos = 0;

        if (mode == VfsMode::Write)
        {
            materialize = true;
            dirty = true;
            if (owns)
                baseline = &local;
        }
        else if (found && manifest.kind == VfsKind::File)
        {
            logical = manifest;
            if (owns)
                baseline = &local;
            pos = mode == VfsMode::Append ? manifest.size : 0;
        }
        else
        {
            materialize = true;
            dirty = true;
        }

        return file.prepare(slot->store, slot->store, logical, baseline, mode, materialize, dirty, pos);
    }

    VfsResult Vfs::readAll(const String& path, std::vector<u8_t>& out)
    {
        VfsFile file;
        VfsResult result = open(path, VfsMode::Read, file);
        if (result != VfsResult::Ok)
            return result;
        out.assign((size_t) file.size(), 0);
        u64_t count = 0;
        if (file.size() > 0)
        {
            result = file.read(out.data(), file.size(), count);
            if (result != VfsResult::Ok)
            {
                file.close();
                return result;
            }
        }
        return file.close();
    }

    VfsResult Vfs::writeAll(const String& path, const void* data, u64_t size)
    {
        VfsFile file;
        VfsResult result = open(path, VfsMode::Write, file);
        if (result != VfsResult::Ok)
            return result;
        u64_t count = 0;
        result = file.write(data, size, count);
        if (result != VfsResult::Ok)
        {
            file.close();
            return result;
        }
        return file.close();
    }

    VfsResult Vfs::readText(const String& path, String& text)
    {
        VfsFile file;
        VfsResult result = open(path, VfsMode::Read, file);
        if (result != VfsResult::Ok)
            return result;
        result = file.readText(text);
        VfsResult closed = file.close();
        if (result != VfsResult::Ok)
            return result;
        return closed;
    }

    VfsResult Vfs::writeText(const String& path, const String& text)
    {
        VfsFile file;
        VfsResult result = open(path, VfsMode::Write, file);
        if (result != VfsResult::Ok)
            return result;
        result = file.writeText(text);
        if (result != VfsResult::Ok)
        {
            file.close();
            return result;
        }
        return file.close();
    }

    VfsResult Vfs::diff(const String& left, const String& right, VfsDiff& out)
    {
        String leftPath;
        String rightPath;
        VfsResult result = VfsPath::normalize(left, leftPath);
        if (result != VfsResult::Ok)
            return result;
        result = VfsPath::normalize(right, rightPath);
        if (result != VfsResult::Ok)
            return result;
        Slot* leftSlot = nullptr;
        Slot* rightSlot = nullptr;
        VfsManifest leftManifest;
        VfsManifest rightManifest;
        result = resolve(leftPath, leftSlot, leftManifest);
        if (result != VfsResult::Ok)
            return result;
        result = resolve(rightPath, rightSlot, rightManifest);
        if (result != VfsResult::Ok)
            return result;
        out = diffManifest(leftManifest, rightManifest);
        return VfsResult::Ok;
    }

    VfsResult Vfs::verify(const String& path, std::vector<u32_t>& badChunks)
    {
        String normal;
        VfsResult result = VfsPath::normalize(path, normal);
        if (result != VfsResult::Ok)
            return result;
        Slot* slot = nullptr;
        VfsManifest manifest;
        result = resolve(normal, slot, manifest);
        if (result != VfsResult::Ok)
            return result;
        return slot->store->verify(manifest, badChunks);
    }
}
