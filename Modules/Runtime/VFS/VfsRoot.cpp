#include "VfsRoot.h"
#include "VfsPath.h"
#include "VfsScheme.h"
#include "VfsVolume.h"
#include <map>

namespace eokas
{
    struct VfsRoot::Mount
    {
        String path;
        bool writable = false;
        std::shared_ptr<VfsVolume> volume;
        std::shared_ptr<VfsScheme> scheme;
    };

    static bool covers(const String& mounted, const String& path)
    {
        if (mounted == "/")
            return true;
        if (path == mounted)
            return true;
        String prefix = mounted + "/";
        return path.startsWith(prefix);
    }

    static String volumePath(const String& mount, const String& full)
    {
        if (full == mount)
            return "/";
        if (mount == "/")
            return full;
        return String("/") + full.substr(mount.length() + 1);
    }

    static VfsResult finish(VfsFile& file, VfsResult result)
    {
        if (result != VfsResult::Ok)
        {
            file.close();
            return result;
        }
        return file.close();
    }

    static void blankRoot(VfsStat& out)
    {
        out.kind = VfsKind::Dir;
        out.size = 0;
        out.mtime = 0;
    }

    static VfsResult fillStat(VfsScheme& scheme, const String& path, VfsStat& out)
    {
        VfsKind kind;
        u64_t size = 0;
        i64_t mtime = 0;
        VfsResult result = scheme.stat(path, kind, size, mtime);
        if (result != VfsResult::Ok)
            return result;
        out.kind = kind;
        out.size = size;
        out.mtime = mtime;
        return VfsResult::Ok;
    }

    VfsRoot::VfsRoot()
    {
    }

    VfsRoot::~VfsRoot()
    {
        clearMounts();
    }

    void VfsRoot::clearMounts()
    {
        for (auto* mount : mMounts)
            delete mount;
        mMounts.clear();
    }

    VfsRoot::Mount* VfsRoot::findMount(const String& path) const
    {
        for (auto* mount : mMounts)
        {
            if (mount->path == path)
                return mount;
        }
        return nullptr;
    }

    VfsRoot::Mount* VfsRoot::matchMount(const String& path) const
    {
        Mount* best = nullptr;
        for (auto* mount : mMounts)
        {
            if (!covers(mount->path, path))
                continue;
            if (best == nullptr || mount->path.length() > best->path.length())
                best = mount;
        }
        return best;
    }

    VfsResult VfsRoot::mount(const String& path, std::unique_ptr<VfsVolume> volume)
    {
        String normal;
        VfsResult result = VfsPath::normalize(path, normal);
        if (result != VfsResult::Ok)
            return result;
        if (volume == nullptr)
            return VfsResult::Io;
        std::unique_ptr<VfsScheme> scheme = volume->takeScheme();
        if (scheme == nullptr)
            return VfsResult::Io;
        bool writable = volume->writable();
        if (findMount(normal) != nullptr)
            return VfsResult::AlreadyExists;
        String cursor = normal;
        while (cursor != "/")
        {
            Mount* owner = matchMount(cursor);
            if (owner != nullptr)
            {
                VfsStat stat;
                VfsResult read = fillStat(*owner->scheme, volumePath(owner->path, cursor), stat);
                if (read == VfsResult::Ok && stat.kind == VfsKind::File)
                    return VfsResult::AlreadyExists;
                if (read == VfsResult::Access)
                    return VfsResult::AlreadyExists;
                if (read != VfsResult::Ok && read != VfsResult::NotFound)
                    return read;
            }
            cursor = VfsPath::parent(cursor);
        }
        auto stored = std::shared_ptr<VfsVolume>(volume.release());
        std::shared_ptr<VfsScheme> opened(scheme.release());
        VfsResult openedResult = opened->open(stored, writable);
        if (openedResult != VfsResult::Ok)
            return openedResult;
        Mount* mounted = new Mount();
        mounted->path = normal;
        mounted->writable = writable;
        mounted->volume = stored;
        mounted->scheme = opened;
        mMounts.push_back(mounted);
        return VfsResult::Ok;
    }

    VfsResult VfsRoot::unmount(const String& path)
    {
        String normal;
        VfsResult result = VfsPath::normalize(path, normal);
        if (result != VfsResult::Ok)
            return result;
        for (auto iter = mMounts.begin(); iter != mMounts.end(); ++iter)
        {
            if ((*iter)->path != normal)
                continue;
            delete *iter;
            mMounts.erase(iter);
            return VfsResult::Ok;
        }
        return VfsResult::NotFound;
    }

    VfsResult VfsRoot::exists(const String& path, bool& out)
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
        Mount* owner = matchMount(normal);
        if (owner == nullptr)
        {
            out = false;
            return VfsResult::Ok;
        }
        return owner->scheme->exists(volumePath(owner->path, normal), out);
    }

    VfsResult VfsRoot::stat(const String& path, VfsStat& out)
    {
        String normal;
        VfsResult result = VfsPath::normalize(path, normal);
        if (result != VfsResult::Ok)
            return result;
        Mount* owner = matchMount(normal);
        if (owner == nullptr)
        {
            if (normal == "/")
            {
                blankRoot(out);
                return VfsResult::Ok;
            }
            return VfsResult::NotFound;
        }
        return fillStat(*owner->scheme, volumePath(owner->path, normal), out);
    }

    VfsResult VfsRoot::list(const String& path, std::vector<VfsDirEntry>& out)
    {
        out.clear();
        String normal;
        VfsResult result = VfsPath::normalize(path, normal);
        if (result != VfsResult::Ok)
            return result;
        Mount* mount = matchMount(normal);
        if (normal != "/")
        {
            if (mount == nullptr)
                return VfsResult::NotFound;
            VfsStat stat;
            result = fillStat(*mount->scheme, volumePath(mount->path, normal), stat);
            if (result != VfsResult::Ok)
                return result;
            if (stat.kind != VfsKind::Dir)
                return VfsResult::Access;
        }
        std::map<String, VfsDirEntry> merged;
        if (mount != nullptr)
        {
            std::vector<VfsDirEntry> entries;
            VfsResult listed = mount->scheme->list(volumePath(mount->path, normal), entries);
            if (listed != VfsResult::Ok)
                return listed;
            for (auto& entry : entries)
                merged[entry.name] = entry;
        }
        for (auto* child : mMounts)
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

    VfsResult VfsRoot::createDir(const String& path)
    {
        String normal;
        VfsResult result = VfsPath::normalize(path, normal);
        if (result != VfsResult::Ok)
            return result;
        if (normal == "/" || findMount(normal) != nullptr)
            return VfsResult::Ok;
        Mount* writer = matchMount(normal);
        if (writer == nullptr)
            return VfsResult::NotFound;
        if (!writer->writable)
            return VfsResult::Access;
        return writer->scheme->createDir(volumePath(writer->path, normal));
    }

    VfsResult VfsRoot::remove(const String& path)
    {
        String normal;
        VfsResult result = VfsPath::normalize(path, normal);
        if (result != VfsResult::Ok)
            return result;
        if (normal == "/" || findMount(normal) != nullptr)
            return VfsResult::Access;
        Mount* owner = matchMount(normal);
        if (owner == nullptr)
            return VfsResult::NotFound;
        VfsStat stat;
        result = fillStat(*owner->scheme, volumePath(owner->path, normal), stat);
        if (result != VfsResult::Ok)
            return result;
        if (!owner->writable)
            return VfsResult::Access;
        if (stat.kind == VfsKind::Dir)
        {
            std::vector<VfsDirEntry> children;
            VfsResult listed = list(normal, children);
            if (listed != VfsResult::Ok)
                return listed;
            if (!children.empty())
                return VfsResult::NotEmpty;
        }
        return owner->scheme->remove(volumePath(owner->path, normal));
    }

    VfsResult VfsRoot::open(const String& path, VfsMode mode, VfsFile& file)
    {
        String normal;
        VfsResult result = VfsPath::normalize(path, normal);
        if (result != VfsResult::Ok)
            return result;
        if (normal == "/" || findMount(normal) != nullptr)
            return VfsResult::Access;
        Mount* owner = matchMount(normal);
        if (owner == nullptr)
            return VfsResult::NotFound;
        if (mode != VfsMode::Read && !owner->writable)
            return VfsResult::Access;
        return owner->scheme->openFile(volumePath(owner->path, normal), mode, file);
    }

    VfsResult VfsRoot::readAll(const String& path, std::vector<u8_t>& out)
    {
        VfsFile file;
        VfsResult result = open(path, VfsMode::Read, file);
        if (result != VfsResult::Ok)
            return result;
        out.assign((size_t) file.size(), 0);
        u64_t count = 0;
        if (file.size() > 0)
            result = file.read(out.data(), file.size(), count);
        return finish(file, result);
    }

    VfsResult VfsRoot::writeAll(const String& path, const void* data, u64_t size)
    {
        VfsFile file;
        VfsResult result = open(path, VfsMode::Write, file);
        if (result != VfsResult::Ok)
            return result;
        u64_t count = 0;
        result = file.write(data, size, count);
        return finish(file, result);
    }

    VfsResult VfsRoot::readText(const String& path, String& text)
    {
        VfsFile file;
        VfsResult result = open(path, VfsMode::Read, file);
        if (result != VfsResult::Ok)
            return result;
        result = file.readText(text);
        return finish(file, result);
    }

    VfsResult VfsRoot::writeText(const String& path, const String& text)
    {
        VfsFile file;
        VfsResult result = open(path, VfsMode::Write, file);
        if (result != VfsResult::Ok)
            return result;
        result = file.writeText(text);
        return finish(file, result);
    }

    VfsResult VfsRoot::rename(const String& from, const String& to)
    {
        String fromPath;
        String toPath;
        VfsResult result = VfsPath::normalize(from, fromPath);
        if (result != VfsResult::Ok)
            return result;
        result = VfsPath::normalize(to, toPath);
        if (result != VfsResult::Ok)
            return result;
        if (fromPath == "/" || toPath == "/")
            return VfsResult::Access;
        if (findMount(fromPath) != nullptr || findMount(toPath) != nullptr)
            return VfsResult::Access;
        Mount* fromMount = matchMount(fromPath);
        Mount* toMount = matchMount(toPath);
        if (fromMount == nullptr || toMount == nullptr)
            return VfsResult::NotFound;
        if (fromMount != toMount)
            return VfsResult::Unsupported;
        if (!fromMount->writable)
            return VfsResult::Access;
        return fromMount->scheme->rename(volumePath(fromMount->path, fromPath), volumePath(fromMount->path, toPath));
    }

    VfsResult VfsRoot::link(const String& existing, const String& path)
    {
        String fromPath;
        String toPath;
        VfsResult result = VfsPath::normalize(existing, fromPath);
        if (result != VfsResult::Ok)
            return result;
        result = VfsPath::normalize(path, toPath);
        if (result != VfsResult::Ok)
            return result;
        if (fromPath == "/" || toPath == "/")
            return VfsResult::Access;
        if (findMount(fromPath) != nullptr || findMount(toPath) != nullptr)
            return VfsResult::Access;
        Mount* fromMount = matchMount(fromPath);
        Mount* toMount = matchMount(toPath);
        if (fromMount == nullptr || toMount == nullptr)
            return VfsResult::NotFound;
        if (fromMount != toMount)
            return VfsResult::Unsupported;
        if (!fromMount->writable)
            return VfsResult::Access;
        return fromMount->scheme->link(volumePath(fromMount->path, fromPath), volumePath(fromMount->path, toPath));
    }
}
