#include "VfsBlockScheme.h"
#include "VfsPath.h"
#include "VfsVolume.h"
#include <cstring>
#include <ctime>
#include <cstdio>

namespace eokas
{
    static const u32_t kSuperMagic = 0x52555053u;
    static const u32_t kInodeMagic = 0x444F4E49u;
    static const u32_t kDirMagic = 0x52494442u;
    static const u16_t kVersion = 1;
    static const char* kSuperKey = "s/super";

    static String dec(u64_t value)
    {
        char text[32];
        std::snprintf(text, sizeof(text), "%llu", (unsigned long long) value);
        return String(text);
    }

    static String inoKey(u64_t ino)
    {
        return String("i/") + dec(ino);
    }

    static String dirKey(u64_t ino)
    {
        return String("d/") + dec(ino);
    }

    static String blockKey(u64_t ino, u64_t index)
    {
        return String("b/") + dec(ino) + "/" + dec(index);
    }

    static i64_t now()
    {
        return (i64_t) std::time(nullptr);
    }

    static VfsResult volumeOccupied(VfsVolume& volume, bool& occupied)
    {
        occupied = false;
        const char* letters = "0123456789abcdefghijklmnopqrstuvwxyz";
        for (size_t index = 0; letters[index] != '\0'; ++index)
        {
            std::vector<String> keys;
            VfsResult result = volume.list(String(letters[index]), keys);
            if (result != VfsResult::Ok)
                return result;
            if (!keys.empty())
            {
                occupied = true;
                return VfsResult::Ok;
            }
        }
        return VfsResult::Ok;
    }

    static bool plainName(const String& name)
    {
        return !name.isEmpty() && !name.contains("/");
    }

    static void splitRelative(const String& relative, std::vector<String>& out)
    {
        out.clear();
        if (relative == "/")
            return;
        size_t cursor = 1;
        while (cursor <= relative.length())
        {
            size_t slash = relative.find('/', cursor);
            String segment = slash == String::npos ? relative.substr(cursor) : relative.substr(cursor, slash - cursor);
            out.push_back(segment);
            if (slash == String::npos)
                break;
            cursor = slash + 1;
        }
    }

    struct Buf
    {
        std::vector<u8_t> data;
        void add(u8_t value) { data.push_back(value); }
        void u16(u16_t value)
        {
            add((u8_t) (value & 0xff));
            add((u8_t) ((value >> 8) & 0xff));
        }
        void u32(u32_t value)
        {
            for (int shift = 0; shift < 32; shift += 8)
                add((u8_t) ((value >> shift) & 0xff));
        }
        void u64(u64_t value)
        {
            for (int shift = 0; shift < 64; shift += 8)
                add((u8_t) ((value >> shift) & 0xff));
        }
    };

    struct View
    {
        const u8_t* p;
        u64_t n;
        u64_t i;
        bool take(void* dst, u64_t count)
        {
            if (i + count > n)
                return false;
            if (count > 0)
                std::memcpy(dst, p + i, (size_t) count);
            i += count;
            return true;
        }
        bool u8(u8_t& value) { return take(&value, 1); }
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
            for (int shift = 0; shift < 8; ++shift)
                value |= ((u64_t) bytes[shift]) << (8 * shift);
            return true;
        }
    };

    VfsBlockScheme::VfsBlockScheme()
    {
    }

    VfsResult VfsBlockScheme::open(std::shared_ptr<VfsVolume> volume, bool writable)
    {
        if (volume == nullptr)
            return VfsResult::Io;
        mVolume = std::move(volume);
        std::vector<u8_t> bytes;
        VfsResult result = mVolume->get(kSuperKey, bytes);
        if (result == VfsResult::Ok)
        {
            if (!decodeSuper(bytes) || mBlockSize == 0 || mNextIno < 2)
                return VfsResult::Corrupt;
            return replay();
        }
        if (result != VfsResult::NotFound)
            return result;
        bool occupied = false;
        result = volumeOccupied(*mVolume, occupied);
        if (result != VfsResult::Ok)
            return result;
        if (occupied)
            return VfsResult::Corrupt;
        if (!writable)
            return VfsResult::NotFound;
        return format();
    }

    VfsResult VfsBlockScheme::format()
    {
        mBlockSize = 65536;
        mNextIno = 2;
        mOrphans.clear();
        VfsResult result = writeSuper();
        if (result != VfsResult::Ok)
            return result;
        Node root;
        root.ino = 1;
        root.kind = VfsKind::Dir;
        root.nlink = 1;
        root.size = 0;
        root.mtime = now();
        result = writeInode(root);
        if (result != VfsResult::Ok)
            return result;
        return writeDir(1, std::vector<DirItem>());
    }

    VfsResult VfsBlockScheme::replay()
    {
        std::vector<u64_t> pending = mOrphans;
        if (pending.empty())
            return VfsResult::Ok;
        for (auto ino : pending)
        {
            if (ino == 1)
                continue;
            Node node;
            VfsResult result = readInode(ino, node);
            if (result == VfsResult::NotFound)
                continue;
            if (result != VfsResult::Ok)
                return result;
            if (node.nlink > 0)
                continue;
            result = freeInode(ino);
            if (result != VfsResult::Ok)
                return result;
        }
        mOrphans.clear();
        return writeSuper();
    }

    bool VfsBlockScheme::decodeSuper(const std::vector<u8_t>& bytes)
    {
        View view;
        view.p = bytes.data();
        view.n = bytes.size();
        view.i = 0;
        u32_t magic = 0;
        u16_t version = 0;
        u32_t count = 0;
        if (!view.u32(magic) || magic != kSuperMagic)
            return false;
        if (!view.u16(version) || version != kVersion)
            return false;
        if (!view.u32(mBlockSize) || !view.u64(mNextIno) || !view.u32(count))
            return false;
        mOrphans.clear();
        for (u32_t index = 0; index < count; ++index)
        {
            u64_t ino = 0;
            if (!view.u64(ino))
                return false;
            mOrphans.push_back(ino);
        }
        return true;
    }

    VfsResult VfsBlockScheme::writeSuper()
    {
        Buf buf;
        buf.u32(kSuperMagic);
        buf.u16(kVersion);
        buf.u32(mBlockSize);
        buf.u64(mNextIno);
        buf.u32((u32_t) mOrphans.size());
        for (auto ino : mOrphans)
            buf.u64(ino);
        return mVolume->put(kSuperKey, buf.data.data(), buf.data.size());
    }

    VfsResult VfsBlockScheme::readInode(u64_t ino, Node& out)
    {
        std::vector<u8_t> bytes;
        VfsResult result = mVolume->get(inoKey(ino), bytes);
        if (result != VfsResult::Ok)
            return result;
        View view;
        view.p = bytes.data();
        view.n = bytes.size();
        view.i = 0;
        u32_t magic = 0;
        u8_t kind = 0;
        u32_t nlink = 0;
        u64_t size = 0;
        u64_t mtime = 0;
        if (!view.u32(magic) || magic != kInodeMagic)
            return VfsResult::Corrupt;
        if (!view.u8(kind) || !view.u32(nlink) || !view.u64(size) || !view.u64(mtime))
            return VfsResult::Corrupt;
        if (kind != (u8_t) VfsKind::File && kind != (u8_t) VfsKind::Dir)
            return VfsResult::Corrupt;
        out.ino = ino;
        out.kind = (VfsKind) kind;
        out.nlink = nlink;
        out.size = size;
        out.mtime = (i64_t) mtime;
        return VfsResult::Ok;
    }

    VfsResult VfsBlockScheme::writeInode(const Node& node)
    {
        Buf buf;
        buf.u32(kInodeMagic);
        buf.add((u8_t) node.kind);
        buf.u32(node.nlink);
        buf.u64(node.size);
        buf.u64((u64_t) node.mtime);
        return mVolume->put(inoKey(node.ino), buf.data.data(), buf.data.size());
    }

    VfsResult VfsBlockScheme::readDir(u64_t ino, std::vector<DirItem>& out)
    {
        out.clear();
        std::vector<u8_t> bytes;
        VfsResult result = mVolume->get(dirKey(ino), bytes);
        if (result == VfsResult::NotFound)
            return VfsResult::Ok;
        if (result != VfsResult::Ok)
            return result;
        View view;
        view.p = bytes.data();
        view.n = bytes.size();
        view.i = 0;
        u32_t magic = 0;
        u32_t count = 0;
        if (!view.u32(magic) || magic != kDirMagic || !view.u32(count))
            return VfsResult::Corrupt;
        for (u32_t index = 0; index < count; ++index)
        {
            u16_t length = 0;
            if (!view.u16(length))
                return VfsResult::Corrupt;
            String name;
            for (u16_t byte = 0; byte < length; ++byte)
            {
                u8_t value = 0;
                if (!view.u8(value))
                    return VfsResult::Corrupt;
                name = name + String((char) value);
            }
            u64_t child = 0;
            u8_t kind = 0;
            if (!view.u64(child) || !view.u8(kind))
                return VfsResult::Corrupt;
            if (kind != (u8_t) VfsKind::File && kind != (u8_t) VfsKind::Dir)
                return VfsResult::Corrupt;
            DirItem item;
            item.name = name;
            item.ino = child;
            item.kind = (VfsKind) kind;
            out.push_back(item);
        }
        return VfsResult::Ok;
    }

    VfsResult VfsBlockScheme::writeDir(u64_t ino, const std::vector<DirItem>& entries)
    {
        Buf buf;
        buf.u32(kDirMagic);
        buf.u32((u32_t) entries.size());
        for (auto& entry : entries)
        {
            if (entry.name.length() > 65535)
                return VfsResult::InvalidPath;
            buf.u16((u16_t) entry.name.length());
            for (size_t index = 0; index < entry.name.length(); ++index)
                buf.add((u8_t) entry.name.at(index));
            buf.u64(entry.ino);
            buf.add((u8_t) entry.kind);
        }
        return mVolume->put(dirKey(ino), buf.data.data(), buf.data.size());
    }

    VfsResult VfsBlockScheme::allocIno(u64_t& ino)
    {
        if (mNextIno < 2)
            return VfsResult::Corrupt;
        u64_t next = mNextIno;
        mNextIno += 1;
        mOrphans.push_back(next);
        VfsResult result = writeSuper();
        if (result != VfsResult::Ok)
        {
            mNextIno -= 1;
            mOrphans.pop_back();
            return result;
        }
        ino = next;
        return VfsResult::Ok;
    }

    VfsResult VfsBlockScheme::addOrphan(u64_t ino)
    {
        for (auto id : mOrphans)
        {
            if (id == ino)
                return VfsResult::Ok;
        }
        mOrphans.push_back(ino);
        return writeSuper();
    }

    VfsResult VfsBlockScheme::removeOrphan(u64_t ino)
    {
        std::vector<u64_t> kept;
        bool found = false;
        for (auto id : mOrphans)
        {
            if (id == ino)
                found = true;
            else
                kept.push_back(id);
        }
        if (!found)
            return VfsResult::Ok;
        mOrphans.swap(kept);
        return writeSuper();
    }

    VfsResult VfsBlockScheme::lookup(const String& path, Node& out)
    {
        std::vector<String> parts;
        splitRelative(path, parts);
        Node node;
        VfsResult result = readInode(1, node);
        if (result != VfsResult::Ok)
            return result;
        node.parent = 0;
        node.name = String();
        if (parts.empty())
        {
            out = node;
            return VfsResult::Ok;
        }
        u64_t current = 1;
        for (size_t index = 0; index < parts.size(); ++index)
        {
            std::vector<DirItem> entries;
            result = readDir(current, entries);
            if (result != VfsResult::Ok)
                return result;
            bool hit = false;
            DirItem item;
            for (auto& entry : entries)
            {
                if (entry.name == parts[index])
                {
                    item = entry;
                    hit = true;
                    break;
                }
            }
            if (!hit)
                return VfsResult::NotFound;
            Node child;
            result = readInode(item.ino, child);
            if (result == VfsResult::NotFound)
            {
                forgetEntry(current, parts[index]);
                return VfsResult::NotFound;
            }
            if (result != VfsResult::Ok)
                return result;
            if (index + 1 < parts.size() && child.kind != VfsKind::Dir)
                return VfsResult::Access;
            child.parent = current;
            child.name = parts[index];
            current = child.ino;
            node = child;
        }
        out = node;
        return VfsResult::Ok;
    }

    VfsResult VfsBlockScheme::forgetEntry(u64_t parent, const String& name)
    {
        std::vector<DirItem> entries;
        VfsResult result = readDir(parent, entries);
        if (result != VfsResult::Ok)
            return result;
        std::vector<DirItem> kept;
        for (auto& entry : entries)
        {
            if (entry.name != name)
                kept.push_back(entry);
        }
        return writeDir(parent, kept);
    }

    VfsResult VfsBlockScheme::linkEntry(u64_t parent, const String& name, u64_t ino, VfsKind kind)
    {
        if (!plainName(name))
            return VfsResult::InvalidPath;
        std::vector<DirItem> entries;
        VfsResult result = readDir(parent, entries);
        if (result != VfsResult::Ok)
            return result;
        for (auto& entry : entries)
        {
            if (entry.name == name)
                return VfsResult::AlreadyExists;
        }
        DirItem item;
        item.name = name;
        item.ino = ino;
        item.kind = kind;
        entries.push_back(item);
        result = writeDir(parent, entries);
        if (result != VfsResult::Ok)
            return result;
        Node node;
        result = readInode(ino, node);
        if (result != VfsResult::Ok)
            return result;
        node.nlink += 1;
        node.mtime = now();
        result = writeInode(node);
        if (result != VfsResult::Ok)
            return result;
        return removeOrphan(ino);
    }

    VfsResult VfsBlockScheme::unlinkEntry(u64_t parent, const String& name)
    {
        std::vector<DirItem> entries;
        VfsResult result = readDir(parent, entries);
        if (result != VfsResult::Ok)
            return result;
        DirItem target;
        bool found = false;
        std::vector<DirItem> kept;
        for (auto& entry : entries)
        {
            if (entry.name == name)
            {
                target = entry;
                found = true;
            }
            else
            {
                kept.push_back(entry);
            }
        }
        if (!found)
            return VfsResult::NotFound;
        Node node;
        result = readInode(target.ino, node);
        if (result != VfsResult::Ok)
            return result;
        if (node.nlink == 0)
            return VfsResult::Corrupt;
        bool dying = node.nlink == 1;
        if (dying)
        {
            result = addOrphan(node.ino);
            if (result != VfsResult::Ok)
                return result;
        }
        node.nlink -= 1;
        node.mtime = now();
        result = writeInode(node);
        if (result != VfsResult::Ok)
            return result;
        result = writeDir(parent, kept);
        if (result != VfsResult::Ok)
            return result;
        if (!dying)
            return VfsResult::Ok;
        if (mOpen.find(node.ino) != mOpen.end())
            return VfsResult::Ok;
        result = freeInode(node.ino);
        if (result != VfsResult::Ok)
            return result;
        return removeOrphan(node.ino);
    }

    VfsResult VfsBlockScheme::moveEntry(u64_t srcParent, const String& srcName, u64_t dstParent, const String& dstName)
    {
        if (!plainName(dstName))
            return VfsResult::InvalidPath;
        std::vector<DirItem> srcEntries;
        VfsResult result = readDir(srcParent, srcEntries);
        if (result != VfsResult::Ok)
            return result;
        if (srcParent == dstParent)
        {
            bool found = false;
            for (auto& entry : srcEntries)
            {
                if (entry.name == dstName)
                    return VfsResult::AlreadyExists;
            }
            for (auto& entry : srcEntries)
            {
                if (entry.name == srcName)
                {
                    entry.name = dstName;
                    found = true;
                    break;
                }
            }
            if (!found)
                return VfsResult::NotFound;
            return writeDir(srcParent, srcEntries);
        }
        DirItem moved;
        bool found = false;
        std::vector<DirItem> kept;
        for (auto& entry : srcEntries)
        {
            if (entry.name == srcName)
            {
                moved = entry;
                found = true;
            }
            else
            {
                kept.push_back(entry);
            }
        }
        if (!found)
            return VfsResult::NotFound;
        result = writeDir(srcParent, kept);
        if (result != VfsResult::Ok)
            return result;
        std::vector<DirItem> dstEntries;
        result = readDir(dstParent, dstEntries);
        if (result != VfsResult::Ok)
            return result;
        for (auto& entry : dstEntries)
        {
            if (entry.name == dstName)
                return VfsResult::AlreadyExists;
        }
        moved.name = dstName;
        dstEntries.push_back(moved);
        result = writeDir(dstParent, dstEntries);
        if (result != VfsResult::Ok)
        {
            kept.push_back(moved);
            kept.back().name = srcName;
            writeDir(srcParent, kept);
            return result;
        }
        return VfsResult::Ok;
    }

    VfsResult VfsBlockScheme::createNode(const String& path, VfsKind kind, Node& out)
    {
        Node parent;
        VfsResult result = lookup(VfsPath::parent(path), parent);
        if (result != VfsResult::Ok)
            return result;
        if (parent.kind != VfsKind::Dir)
            return VfsResult::AlreadyExists;
        u64_t ino = 0;
        result = allocIno(ino);
        if (result != VfsResult::Ok)
            return result;
        Node born;
        born.ino = ino;
        born.kind = kind;
        born.nlink = 0;
        born.size = 0;
        born.mtime = now();
        result = writeInode(born);
        if (result != VfsResult::Ok)
            return result;
        result = linkEntry(parent.ino, VfsPath::name(path), ino, kind);
        if (result != VfsResult::Ok)
            return result;
        return readInode(ino, out);
    }

    VfsResult VfsBlockScheme::ensureDir(const String& path)
    {
        if (path == "/")
            return VfsResult::Ok;
        VfsResult parent = ensureDir(VfsPath::parent(path));
        if (parent != VfsResult::Ok)
            return parent;
        Node existing;
        VfsResult result = lookup(path, existing);
        if (result == VfsResult::Ok && existing.kind == VfsKind::Dir)
            return VfsResult::Ok;
        if (result == VfsResult::Ok)
            return VfsResult::AlreadyExists;
        if (result != VfsResult::NotFound)
            return result;
        Node created;
        return createNode(path, VfsKind::Dir, created);
    }

    VfsResult VfsBlockScheme::freeInode(u64_t ino)
    {
        if (ino == 1)
            return VfsResult::Access;
        Node node;
        VfsResult result = readInode(ino, node);
        u64_t size = 0;
        if (result == VfsResult::Ok)
            size = node.size;
        else if (result != VfsResult::NotFound)
            return result;
        if (mBlockSize == 0)
            return VfsResult::Corrupt;
        if (size > 0)
        {
            u64_t count = (size + mBlockSize - 1) / mBlockSize;
            for (u64_t index = 0; index < count; ++index)
            {
                VfsResult dropped = dropBlock(ino, index);
                if (dropped != VfsResult::Ok)
                    return dropped;
            }
        }
        result = mVolume->remove(inoKey(ino));
        if (result != VfsResult::Ok && result != VfsResult::NotFound)
            return result;
        result = mVolume->remove(dirKey(ino));
        if (result != VfsResult::Ok && result != VfsResult::NotFound)
            return result;
        return VfsResult::Ok;
    }

    VfsResult VfsBlockScheme::truncate(u64_t ino)
    {
        Node node;
        VfsResult result = readInode(ino, node);
        if (result != VfsResult::Ok)
            return result;
        if (node.kind != VfsKind::File)
            return VfsResult::Access;
        if (mBlockSize == 0)
            return VfsResult::Corrupt;
        u64_t count = 0;
        if (node.size > 0)
            count = (node.size + mBlockSize - 1) / mBlockSize;
        for (u64_t index = 0; index < count; ++index)
        {
            VfsResult dropped = dropBlock(ino, index);
            if (dropped != VfsResult::Ok)
                return dropped;
        }
        node.size = 0;
        node.mtime = now();
        return writeInode(node);
    }

    VfsResult VfsBlockScheme::stat(const String& path, VfsKind& kind, u64_t& size, i64_t& mtime)
    {
        Node node;
        VfsResult result = lookup(path, node);
        if (result != VfsResult::Ok)
            return result;
        kind = node.kind;
        size = node.size;
        mtime = node.mtime;
        return VfsResult::Ok;
    }

    VfsResult VfsBlockScheme::exists(const String& path, bool& out)
    {
        Node node;
        VfsResult result = lookup(path, node);
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

    VfsResult VfsBlockScheme::list(const String& path, std::vector<VfsDirEntry>& out)
    {
        out.clear();
        Node node;
        VfsResult result = lookup(path, node);
        if (result != VfsResult::Ok)
            return result;
        if (node.kind != VfsKind::Dir)
            return VfsResult::Access;
        std::vector<DirItem> entries;
        result = readDir(node.ino, entries);
        if (result != VfsResult::Ok)
            return result;
        for (auto& entry : entries)
        {
            VfsDirEntry item;
            item.name = entry.name;
            item.kind = entry.kind;
            out.push_back(item);
        }
        return VfsResult::Ok;
    }

    VfsResult VfsBlockScheme::createDir(const String& path)
    {
        return ensureDir(path);
    }

    VfsResult VfsBlockScheme::remove(const String& path)
    {
        Node node;
        VfsResult result = lookup(path, node);
        if (result != VfsResult::Ok)
            return result;
        if (node.ino == 1)
            return VfsResult::Access;
        if (node.kind == VfsKind::Dir)
        {
            std::vector<DirItem> children;
            result = readDir(node.ino, children);
            if (result != VfsResult::Ok)
                return result;
            if (!children.empty())
                return VfsResult::NotEmpty;
        }
        return unlinkEntry(node.parent, node.name);
    }

    VfsResult VfsBlockScheme::rename(const String& from, const String& to)
    {
        if (from == to)
            return VfsResult::Ok;
        Node src;
        VfsResult result = lookup(from, src);
        if (result != VfsResult::Ok)
            return result;
        if (src.ino == 1)
            return VfsResult::Access;
        if (src.kind == VfsKind::Dir)
        {
            String cursor = to;
            while (cursor != "/")
            {
                if (cursor == from)
                    return VfsResult::Access;
                String parent = VfsPath::parent(cursor);
                if (parent == cursor)
                    break;
                cursor = parent;
            }
        }
        Node dst;
        result = lookup(to, dst);
        bool dstExists = result == VfsResult::Ok;
        if (!dstExists && result != VfsResult::NotFound)
            return result;
        Node parent;
        result = lookup(VfsPath::parent(to), parent);
        if (result != VfsResult::Ok)
            return result;
        if (parent.kind != VfsKind::Dir)
            return VfsResult::Access;
        if (dstExists && dst.ino == src.ino)
            return unlinkEntry(src.parent, src.name);
        if (dstExists)
        {
            if (dst.ino == 1 || dst.kind != src.kind)
                return VfsResult::Access;
            if (dst.kind == VfsKind::Dir)
            {
                std::vector<DirItem> children;
                result = readDir(dst.ino, children);
                if (result != VfsResult::Ok)
                    return result;
                if (!children.empty())
                    return VfsResult::NotEmpty;
            }
            result = unlinkEntry(dst.parent, dst.name);
            if (result != VfsResult::Ok)
                return result;
        }
        return moveEntry(src.parent, src.name, parent.ino, VfsPath::name(to));
    }

    VfsResult VfsBlockScheme::link(const String& existing, const String& path)
    {
        Node src;
        VfsResult result = lookup(existing, src);
        if (result != VfsResult::Ok)
            return result;
        if (src.kind != VfsKind::File)
            return VfsResult::Access;
        Node dst;
        result = lookup(path, dst);
        if (result == VfsResult::Ok)
            return VfsResult::AlreadyExists;
        if (result != VfsResult::NotFound)
            return result;
        Node parent;
        result = lookup(VfsPath::parent(path), parent);
        if (result != VfsResult::Ok)
            return result;
        if (parent.kind != VfsKind::Dir)
            return VfsResult::Access;
        return linkEntry(parent.ino, VfsPath::name(path), src.ino, VfsKind::File);
    }

    VfsResult VfsBlockScheme::openFile(const String& path, VfsMode mode, VfsFile& file)
    {
        if (mode != VfsMode::Read)
        {
            VfsResult ensured = ensureDir(VfsPath::parent(path));
            if (ensured != VfsResult::Ok)
                return ensured;
        }
        Node node;
        VfsResult result = lookup(path, node);
        bool found = result == VfsResult::Ok;
        if (!found && result != VfsResult::NotFound)
            return result;
        if (mode == VfsMode::Read)
        {
            if (!found)
                return VfsResult::NotFound;
            if (node.kind != VfsKind::File)
                return VfsResult::Access;
        }
        else
        {
            if (found && node.kind == VfsKind::Dir)
                return VfsResult::AlreadyExists;
            if (!found)
            {
                result = createNode(path, VfsKind::File, node);
                if (result != VfsResult::Ok)
                    return result;
            }
            if (mode == VfsMode::Write)
            {
                result = truncate(node.ino);
                if (result != VfsResult::Ok)
                    return result;
                node.size = 0;
            }
        }
        u64_t pos = mode == VfsMode::Append ? node.size : 0;
        noteOpen(node.ino);
        auto self = std::static_pointer_cast<VfsBlockScheme>(shared_from_this());
        auto body = std::make_unique<VfsBlockSchemeFile>(self, node.ino, mode, node.size, mBlockSize, pos);
        result = bind(file, std::move(body));
        if (result != VfsResult::Ok)
        {
            noteClose(node.ino);
            return result;
        }
        return VfsResult::Ok;
    }

    void VfsBlockScheme::noteOpen(u64_t ino)
    {
        mOpen[ino] += 1;
    }

    void VfsBlockScheme::noteClose(u64_t ino)
    {
        auto found = mOpen.find(ino);
        if (found != mOpen.end())
        {
            if (found->second > 0)
                found->second -= 1;
            if (found->second == 0)
                mOpen.erase(found);
        }
        if (mOpen.find(ino) != mOpen.end())
            return;
        Node node;
        VfsResult result = readInode(ino, node);
        if (result == VfsResult::NotFound)
        {
            removeOrphan(ino);
            return;
        }
        if (result != VfsResult::Ok || node.nlink > 0)
            return;
        if (freeInode(ino) != VfsResult::Ok)
            return;
        removeOrphan(ino);
    }

    VfsResult VfsBlockScheme::readBlock(u64_t ino, u64_t index, std::vector<u8_t>& out)
    {
        return mVolume->get(blockKey(ino, index), out);
    }

    VfsResult VfsBlockScheme::writeBlock(u64_t ino, u64_t index, const void* data, u64_t size)
    {
        if (size > 0 && data == nullptr)
            return VfsResult::Io;
        return mVolume->put(blockKey(ino, index), data, size);
    }

    VfsResult VfsBlockScheme::dropBlock(u64_t ino, u64_t index)
    {
        VfsResult result = mVolume->remove(blockKey(ino, index));
        if (result == VfsResult::NotFound)
            return VfsResult::Ok;
        return result;
    }

    VfsResult VfsBlockScheme::updateFile(u64_t ino, u64_t size)
    {
        Node node;
        VfsResult result = readInode(ino, node);
        if (result != VfsResult::Ok)
            return result;
        if (node.kind != VfsKind::File)
            return VfsResult::Access;
        node.size = size;
        node.mtime = now();
        return writeInode(node);
    }

    VfsBlockSchemeFile::VfsBlockSchemeFile(std::shared_ptr<VfsBlockScheme> block, u64_t ino, VfsMode mode, u64_t size, u32_t blockSize, u64_t pos)
        : mBlock(std::move(block))
        , mIno(ino)
        , mMode(mode)
        , mSize(size)
        , mBlockSize(blockSize)
        , mPos(pos)
        , mOpen(true)
    {
    }

    VfsBlockSchemeFile::~VfsBlockSchemeFile()
    {
        if (!mOpen)
            return;
        flush();
        mBlock->noteClose(mIno);
        mOpen = false;
    }

    bool VfsBlockSchemeFile::isOpen() const { return mOpen; }
    u64_t VfsBlockSchemeFile::tell() const { return mOpen ? mPos : 0; }
    u64_t VfsBlockSchemeFile::size() const { return mOpen ? mSize : 0; }
    VfsMode VfsBlockSchemeFile::mode() const { return mMode; }

    VfsResult VfsBlockSchemeFile::store()
    {
        if (!mDirty || !mValid)
            return VfsResult::Ok;
        u64_t begin = mIndex * mBlockSize;
        if (mSize <= begin)
        {
            VfsResult dropped = mBlock->dropBlock(mIno, mIndex);
            if (dropped != VfsResult::Ok)
                return dropped;
            mDirty = false;
            return VfsResult::Ok;
        }
        u64_t remain = mSize - begin;
        u32_t length = remain > mBlockSize ? mBlockSize : (u32_t) remain;
        VfsResult result = mBlock->writeBlock(mIno, mIndex, mBuf.empty() ? nullptr : mBuf.data(), length);
        if (result != VfsResult::Ok)
            return result;
        mDirty = false;
        return VfsResult::Ok;
    }

    VfsResult VfsBlockSchemeFile::load(u64_t index)
    {
        if (mBlockSize == 0)
            return VfsResult::Corrupt;
        if (mValid && mIndex == index)
            return VfsResult::Ok;
        VfsResult stored = store();
        if (stored != VfsResult::Ok)
            return stored;
        std::vector<u8_t> fresh(mBlockSize, 0);
        u64_t begin = index * mBlockSize;
        u32_t expect = 0;
        if (mSize > begin)
        {
            u64_t remain = mSize - begin;
            expect = remain > mBlockSize ? mBlockSize : (u32_t) remain;
        }
        if (expect > 0)
        {
            std::vector<u8_t> got;
            VfsResult result = mBlock->readBlock(mIno, index, got);
            if (result != VfsResult::Ok && result != VfsResult::NotFound)
                return result;
            if (result == VfsResult::Ok)
            {
                u32_t copy = (u32_t) got.size();
                if (copy > expect)
                    copy = expect;
                if (copy > 0)
                    std::memcpy(fresh.data(), got.data(), copy);
            }
        }
        mBuf.swap(fresh);
        mIndex = index;
        mValid = true;
        mDirty = false;
        return VfsResult::Ok;
    }

    VfsResult VfsBlockSchemeFile::flush()
    {
        if (!mOpen)
            return VfsResult::Closed;
        if (mMode == VfsMode::Read)
            return VfsResult::Ok;
        VfsResult stored = store();
        if (stored != VfsResult::Ok)
            return stored;
        if (!mMetaDirty)
            return VfsResult::Ok;
        VfsResult result = mBlock->updateFile(mIno, mSize);
        if (result != VfsResult::Ok)
            return result;
        mMetaDirty = false;
        return VfsResult::Ok;
    }

    VfsResult VfsBlockSchemeFile::close()
    {
        if (!mOpen)
            return VfsResult::Ok;
        VfsResult flushed = flush();
        if (flushed != VfsResult::Ok)
            return flushed;
        mBlock->noteClose(mIno);
        mOpen = false;
        return VfsResult::Ok;
    }

    VfsResult VfsBlockSchemeFile::seek(i64_t offset, VfsSeek origin)
    {
        if (!mOpen)
            return VfsResult::Closed;
        i64_t target = 0;
        if (origin == VfsSeek::Begin)
            target = offset;
        else if (origin == VfsSeek::Current)
            target = (i64_t) mPos + offset;
        else
            target = (i64_t) mSize + offset;
        if (target < 0)
            return VfsResult::InvalidPath;
        mPos = (u64_t) target;
        return VfsResult::Ok;
    }

    VfsResult VfsBlockSchemeFile::read(void* data, u64_t size, u64_t& count)
    {
        count = 0;
        if (!mOpen)
            return VfsResult::Closed;
        if (size == 0 || mPos >= mSize)
            return VfsResult::Ok;
        u64_t remain = mSize - mPos;
        if (size < remain)
            remain = size;
        u8_t* dst = static_cast<u8_t*>(data);
        u64_t done = 0;
        while (done < remain)
        {
            u64_t index = (mPos) / mBlockSize;
            u32_t inner = (u32_t) (mPos % mBlockSize);
            VfsResult result = load(index);
            if (result != VfsResult::Ok)
                return result;
            u32_t room = mBlockSize - inner;
            u32_t take = room;
            if ((u64_t) take > remain - done)
                take = (u32_t) (remain - done);
            if (data != nullptr)
                std::memcpy(dst + done, mBuf.data() + inner, take);
            mPos += take;
            done += take;
        }
        count = done;
        return VfsResult::Ok;
    }

    VfsResult VfsBlockSchemeFile::write(const void* data, u64_t size, u64_t& count)
    {
        count = 0;
        if (!mOpen)
            return VfsResult::Closed;
        if (mMode == VfsMode::Read)
            return VfsResult::Access;
        if (mBlockSize == 0)
            return VfsResult::Corrupt;
        if (size == 0)
            return VfsResult::Ok;
        const u8_t* src = static_cast<const u8_t*>(data);
        u64_t done = 0;
        while (done < size)
        {
            u64_t at = mPos + done;
            u64_t index = at / mBlockSize;
            u32_t inner = (u32_t) (at % mBlockSize);
            VfsResult result = load(index);
            if (result != VfsResult::Ok)
                return result;
            u32_t room = mBlockSize - inner;
            u32_t take = room;
            if ((u64_t) take > size - done)
                take = (u32_t) (size - done);
            if (src != nullptr)
                std::memcpy(mBuf.data() + inner, src + done, take);
            else
                std::memset(mBuf.data() + inner, 0, take);
            mDirty = true;
            mMetaDirty = true;
            u64_t end = at + take;
            if (end > mSize)
                mSize = end;
            done += take;
        }
        mPos += done;
        count = done;
        return VfsResult::Ok;
    }
}
