#include "Unit.h"
#include "VFS/Header.h"

#include <cstring>
#include <thread>
#include <vector>

#if _EOKAS_OS == _EOKAS_OS_WIN64 || _EOKAS_OS == _EOKAS_OS_WIN32
#pragma comment(lib, "ws2_32.lib")
#endif

#if _EOKAS_OS == _EOKAS_OS_WIN64 || _EOKAS_OS == _EOKAS_OS_WIN32 || _EOKAS_OS == _EOKAS_OS_MACOS || _EOKAS_OS == _EOKAS_OS_IOS
#define COVER_SOCKET 1
#else
#define COVER_SOCKET 0
#endif

using namespace eokas;

static String restRoot(const char* tag)
{
    return String::format("vfs-unit-%s-%u", tag, Process::getPID());
}

static bool restHas(const std::vector<VfsDirEntry>& entries, const char* name, VfsKind kind)
{
    for (auto& entry : entries)
    {
        if (entry.name == name)
            return entry.kind == kind;
    }
    return false;
}

static bool restRef(VfsVolume& volume, const String& hash, u32_t expect)
{
    std::vector<u8_t> bytes;
    if (volume.get(String("k/") + hash, bytes) != VfsResult::Ok || bytes.size() != 4)
        return false;
    u32_t count = (u32_t) bytes[0] | ((u32_t) bytes[1] << 8) | ((u32_t) bytes[2] << 16) | ((u32_t) bytes[3] << 24);
    return count == expect;
}

static String restChunk(const String& hash)
{
    return String("c/") + hash.substr(0, 2) + "/" + hash;
}

static String restManifest(const String& path)
{
    return String("m/") + VfsPath::hash(path);
}

static String restDir(const String& path)
{
    return String("d/") + VfsPath::hash(path);
}

namespace
{
    class RestFault : public VfsVolume
    {
    public:
        VfsMemoryVolume inner;
        String putPrefix;
        int putTimes = 0;
        VfsResult putResult = VfsResult::Io;
        String getPrefix;
        int getTimes = 0;
        VfsResult getResult = VfsResult::Io;
        String removePrefix;
        int removeTimes = 0;
        VfsResult removeResult = VfsResult::Io;
        VfsFile* reenter = nullptr;
        int reenterTimes = 0;

        VfsResult exists(const String& key, bool& out) override
        {
            return inner.exists(key, out);
        }

        VfsResult get(const String& key, std::vector<u8_t>& out) override
        {
            if (getTimes > 0 && key.startsWith(getPrefix))
            {
                getTimes -= 1;
                return getResult;
            }
            return inner.get(key, out);
        }

        VfsResult put(const String& key, const void* data, u64_t size) override
        {
            if (reenterTimes > 0 && reenter != nullptr && key.startsWith(String("m/")))
            {
                reenterTimes -= 1;
                u64_t count = 0;
                reenter->write("Z", 1, count);
            }
            if (putTimes > 0 && key.startsWith(putPrefix))
            {
                putTimes -= 1;
                return putResult;
            }
            return inner.put(key, data, size);
        }

        VfsResult remove(const String& key) override
        {
            if (removeTimes > 0 && key.startsWith(removePrefix))
            {
                removeTimes -= 1;
                return removeResult;
            }
            return inner.remove(key);
        }

        VfsResult list(const String& prefix, std::vector<String>& out) override
        {
            return inner.list(prefix, out);
        }
    };

    class RestStatus : public VfsVolume
    {
    public:
        VfsMemoryVolume inner;
        bool forceGet = false;
        VfsResult getResult = VfsResult::Ok;
        bool forcePut = false;
        VfsResult putResult = VfsResult::Ok;
        bool forceRemove = false;
        VfsResult removeResult = VfsResult::Ok;
        bool forceList = false;
        VfsResult listResult = VfsResult::Ok;
        bool forceExists = false;
        VfsResult existsResult = VfsResult::Ok;
        bool existsValue = false;

        VfsResult exists(const String& key, bool& out) override
        {
            if (forceExists)
            {
                if (existsResult == VfsResult::Ok)
                    out = existsValue;
                return existsResult;
            }
            return inner.exists(key, out);
        }

        VfsResult get(const String& key, std::vector<u8_t>& out) override
        {
            if (forceGet)
                return getResult;
            return inner.get(key, out);
        }

        VfsResult put(const String& key, const void* data, u64_t size) override
        {
            if (forcePut)
                return putResult;
            return inner.put(key, data, size);
        }

        VfsResult remove(const String& key) override
        {
            if (forceRemove)
                return removeResult;
            return inner.remove(key);
        }

        VfsResult list(const String& prefix, std::vector<String>& out) override
        {
            if (forceList)
                return listResult;
            return inner.list(prefix, out);
        }
    };

    struct RestJoin
    {
        std::thread& worker;
        Socket* listen = nullptr;

        ~RestJoin()
        {
            if (listen != nullptr)
                listen->close();
            if (worker.joinable())
                worker.join();
        }
    };
}

EOKAS_TEST_CASE(vfsdeny) {
    VfsFile closed;
    u64_t count = 7;
    EOKAS_EXPECT(closed.seek(0, VfsSeek::Begin) == VfsResult::Closed);
    EOKAS_EXPECT(closed.write("a", 1, count) == VfsResult::Closed);
    EOKAS_EXPECT(closed.flush() == VfsResult::Closed);
    String text = "stay";
    EOKAS_EXPECT(closed.readText(text) == VfsResult::Closed);
    EOKAS_EXPECT(text == "stay");
    EOKAS_EXPECT(closed.writeText("a") == VfsResult::Closed);
    EOKAS_EXPECT(closed.tell() == 0);
    EOKAS_EXPECT(closed.size() == 0);
    EOKAS_EXPECT(!closed.isOpen());

    Vfs bare;
    bool present = true;
    VfsStat stat;
    stat.size = 4;
    std::vector<VfsDirEntry> entries;
    entries.push_back(VfsDirEntry{});
    std::vector<u8_t> bytes{1};
    VfsDiff diff;
    diff.sameKind = false;
    std::vector<u32_t> bad{1};
    EOKAS_EXPECT(bare.createDir("rel") == VfsResult::InvalidPath);
    EOKAS_EXPECT(bare.createDir("/missing") == VfsResult::NotFound);
    EOKAS_EXPECT(bare.remove("rel") == VfsResult::InvalidPath);
    EOKAS_EXPECT(bare.remove("/missing") == VfsResult::NotFound);
    EOKAS_EXPECT(bare.remove("/") == VfsResult::Access);
    EOKAS_EXPECT(bare.stat("rel", stat) == VfsResult::InvalidPath);
    EOKAS_EXPECT(stat.size == 4);
    EOKAS_EXPECT(bare.list("rel", entries) == VfsResult::InvalidPath);
    EOKAS_EXPECT(entries.empty());
    VfsFile file;
    EOKAS_EXPECT(bare.open("rel", VfsMode::Read, file) == VfsResult::InvalidPath);
    EOKAS_EXPECT(bare.open("/", VfsMode::Read, file) == VfsResult::Access);
    EOKAS_EXPECT(bare.readAll("rel", bytes) == VfsResult::InvalidPath);
    EOKAS_EXPECT(bare.writeAll("rel", "a", 1) == VfsResult::InvalidPath);
    EOKAS_EXPECT(bare.readText("rel", text) == VfsResult::InvalidPath);
    EOKAS_EXPECT(bare.writeText("rel", "a") == VfsResult::InvalidPath);
    EOKAS_EXPECT(bare.diff("/a", "rel", diff) == VfsResult::InvalidPath);
    EOKAS_EXPECT(bare.diff("rel", "/a", diff) == VfsResult::InvalidPath);
    EOKAS_EXPECT(!diff.sameKind);
    EOKAS_EXPECT(bare.verify("rel", bad) == VfsResult::InvalidPath);
    EOKAS_EXPECT(bad.size() == 1);
    EOKAS_EXPECT(bare.detach("rel") == VfsResult::InvalidPath);
    EOKAS_EXPECT(bare.detach("/missing") == VfsResult::NotFound);
    EOKAS_EXPECT(bare.exists("/missing", present) == VfsResult::Ok);
    EOKAS_EXPECT(!present);

    EOKAS_EXPECT(bare.attach("/", std::make_unique<VfsMemoryVolume>(), true) == VfsResult::Ok);
    EOKAS_EXPECT(bare.createDir("/box") == VfsResult::Ok);
    EOKAS_EXPECT(bare.open("/box", VfsMode::Write, file) == VfsResult::AlreadyExists);
    EOKAS_EXPECT(bare.open("/box", VfsMode::Append, file) == VfsResult::AlreadyExists);
    EOKAS_EXPECT(bare.open("/box", VfsMode::ReadWrite, file) == VfsResult::AlreadyExists);
    EOKAS_EXPECT(bare.writeAll("/box", "a", 1) == VfsResult::AlreadyExists);
    EOKAS_EXPECT(bare.writeText("/box", "a") == VfsResult::AlreadyExists);
    EOKAS_EXPECT(bare.readAll("/box", bytes) == VfsResult::Access);
    EOKAS_EXPECT(bare.readText("/box", text) == VfsResult::Access);
    EOKAS_EXPECT(bare.writeText("/a.txt", "file") == VfsResult::Ok);
    EOKAS_EXPECT(bare.createDir("/a.txt") == VfsResult::AlreadyExists);
    EOKAS_EXPECT(bare.open("/a.txt/child", VfsMode::Write, file) == VfsResult::AlreadyExists);
    EOKAS_EXPECT(bare.writeText("/a.txt/child", "no") == VfsResult::AlreadyExists);
    EOKAS_EXPECT(bare.readAll("/missing", bytes) == VfsResult::NotFound);
    EOKAS_EXPECT(bare.readText("/missing", text) == VfsResult::NotFound);
    EOKAS_EXPECT(bare.writeText("/blank", "") == VfsResult::Ok);
    EOKAS_EXPECT(bare.readText("/blank", text) == VfsResult::Ok);
    EOKAS_EXPECT(text.isEmpty());
    EOKAS_EXPECT(bare.writeAll("/empty", nullptr, 0) == VfsResult::Ok);
    EOKAS_EXPECT(bare.readAll("/empty", bytes) == VfsResult::Ok);
    EOKAS_EXPECT(bytes.empty());
    EOKAS_EXPECT(bare.open("/a.txt", VfsMode::Read, file) == VfsResult::Ok);
    EOKAS_EXPECT(file.writeText("Z") == VfsResult::Access);
    EOKAS_EXPECT(file.flush() == VfsResult::Ok);
    EOKAS_EXPECT(file.close() == VfsResult::Ok);
    EOKAS_EXPECT(bare.readText("/a.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "file");
    EOKAS_EXPECT(bare.detach("/") == VfsResult::Ok);
    EOKAS_EXPECT(bare.detach("/") == VfsResult::NotFound);
    present = true;
    EOKAS_EXPECT(bare.exists("/a.txt", present) == VfsResult::Ok);
    EOKAS_EXPECT(!present);
    return 0;
}

EOKAS_TEST_CASE(vfsfault) {
    auto gate = std::make_unique<RestFault>();
    RestFault* raw = gate.get();
    Vfs vfs;
    EOKAS_EXPECT(vfs.attach("/", std::move(gate), true) == VfsResult::Ok);

    VfsFile held;
    u64_t count = 0;
    EOKAS_EXPECT(vfs.open("/hold.txt", VfsMode::Write, held) == VfsResult::Ok);
    EOKAS_EXPECT(held.write("hi", 2, count) == VfsResult::Ok);
    raw->putPrefix = "c/";
    raw->putTimes = 1;
    raw->putResult = VfsResult::Io;
    EOKAS_EXPECT(held.close() == VfsResult::Io);
    EOKAS_EXPECT(held.isOpen());
    EOKAS_EXPECT(held.size() == 2);
    raw->putTimes = 0;
    EOKAS_EXPECT(held.close() == VfsResult::Ok);
    String text;
    EOKAS_EXPECT(vfs.readText("/hold.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "hi");

    EOKAS_EXPECT(vfs.writeText("/same.txt", "ab") == VfsResult::Ok);
    EOKAS_EXPECT(vfs.open("/same.txt", VfsMode::ReadWrite, held) == VfsResult::Ok);
    EOKAS_EXPECT(held.write(nullptr, 0, count) == VfsResult::Ok);
    raw->putPrefix = "m/";
    raw->putTimes = 1;
    raw->putResult = VfsResult::Io;
    EOKAS_EXPECT(held.close() == VfsResult::Io);
    EOKAS_EXPECT(held.isOpen());
    raw->putTimes = 0;
    EOKAS_EXPECT(held.close() == VfsResult::Ok);
    EOKAS_EXPECT(vfs.readText("/same.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "ab");

    EOKAS_EXPECT(vfs.open("/newref.txt", VfsMode::Write, held) == VfsResult::Ok);
    EOKAS_EXPECT(held.write("qr", 2, count) == VfsResult::Ok);
    raw->putPrefix = "k/";
    raw->putTimes = 1;
    raw->putResult = VfsResult::Io;
    EOKAS_EXPECT(held.flush() == VfsResult::Io);
    EOKAS_EXPECT(vfs.readText("/newref.txt", text) == VfsResult::NotFound);
    raw->putTimes = 0;
    EOKAS_EXPECT(held.flush() == VfsResult::Ok);
    EOKAS_EXPECT(held.close() == VfsResult::Ok);
    EOKAS_EXPECT(vfs.readText("/newref.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "qr");
    EOKAS_EXPECT(restRef(raw->inner, vfsHash("qr", 2), 1));

    EOKAS_EXPECT(vfs.open("/link.txt", VfsMode::Write, held) == VfsResult::Ok);
    EOKAS_EXPECT(held.write("yo", 2, count) == VfsResult::Ok);
    raw->putPrefix = "d/";
    raw->putTimes = 1;
    raw->putResult = VfsResult::Io;
    EOKAS_EXPECT(held.flush() == VfsResult::Io);
    EOKAS_EXPECT(vfs.readText("/link.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "yo");
    std::vector<VfsDirEntry> entries;
    EOKAS_EXPECT(vfs.list("/", entries) == VfsResult::Ok);
    EOKAS_EXPECT(!restHas(entries, "link.txt", VfsKind::File));
    raw->putTimes = 0;
    EOKAS_EXPECT(held.flush() == VfsResult::Ok);
    EOKAS_EXPECT(held.close() == VfsResult::Ok);
    EOKAS_EXPECT(vfs.list("/", entries) == VfsResult::Ok);
    EOKAS_EXPECT(restHas(entries, "link.txt", VfsKind::File));

    EOKAS_EXPECT(vfs.open("/re.txt", VfsMode::Write, held) == VfsResult::Ok);
    EOKAS_EXPECT(held.write("ab", 2, count) == VfsResult::Ok);
    raw->reenter = &held;
    raw->reenterTimes = 1;
    EOKAS_EXPECT(held.flush() == VfsResult::Ok);
    EOKAS_EXPECT(held.close() == VfsResult::Ok);
    raw->reenter = nullptr;
    EOKAS_EXPECT(vfs.readText("/re.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "abZ");

    EOKAS_EXPECT(vfs.writeText("/only.txt", "one") == VfsResult::Ok);
    String only = vfsHash("one", 3);
    EOKAS_EXPECT(restRef(raw->inner, only, 1));
    raw->removePrefix = "k/";
    raw->removeTimes = 1;
    raw->removeResult = VfsResult::Io;
    EOKAS_EXPECT(vfs.remove("/only.txt") == VfsResult::Io);
    bool present = true;
    EOKAS_EXPECT(vfs.exists("/only.txt", present) == VfsResult::Ok);
    EOKAS_EXPECT(!present);
    EOKAS_EXPECT(vfs.remove("/only.txt") == VfsResult::NotFound);
    EOKAS_EXPECT(restRef(raw->inner, only, 1));
    EOKAS_EXPECT(raw->inner.exists(restChunk(only), present) == VfsResult::Ok);
    EOKAS_EXPECT(present);

    EOKAS_EXPECT(vfs.createDir("/box/in") == VfsResult::Ok);
    raw->getPrefix = restDir("/box");
    raw->getTimes = 1;
    raw->getResult = VfsResult::Io;
    EOKAS_EXPECT(vfs.remove("/box") == VfsResult::Io);
    EOKAS_EXPECT(vfs.exists("/box", present) == VfsResult::Ok);
    EOKAS_EXPECT(present);
    raw->getTimes = 0;
    EOKAS_EXPECT(vfs.remove("/box") == VfsResult::NotEmpty);
    EOKAS_EXPECT(vfs.remove("/box/in") == VfsResult::Ok);
    EOKAS_EXPECT(vfs.remove("/box") == VfsResult::Ok);

    raw->getPrefix = restManifest("/fresh");
    raw->getTimes = 1;
    raw->getResult = VfsResult::Io;
    EOKAS_EXPECT(vfs.createDir("/fresh") == VfsResult::Io);
    present = true;
    EOKAS_EXPECT(vfs.exists("/fresh", present) == VfsResult::Ok);
    EOKAS_EXPECT(!present);
    raw->getTimes = 0;
    EOKAS_EXPECT(vfs.createDir("/fresh") == VfsResult::Ok);

    EOKAS_EXPECT(vfs.writeText("/blocked", "x") == VfsResult::Ok);
    raw->getPrefix = restManifest("/blocked");
    raw->getTimes = 1;
    raw->getResult = VfsResult::Io;
    EOKAS_EXPECT(vfs.attach("/blocked/child", std::make_unique<VfsMemoryVolume>(), true) == VfsResult::Io);
    raw->getTimes = 1;
    present = true;
    EOKAS_EXPECT(vfs.exists("/blocked", present) == VfsResult::Io);
    EOKAS_EXPECT(present);
    raw->getTimes = 0;
    EOKAS_EXPECT(vfs.exists("/blocked", present) == VfsResult::Ok);
    EOKAS_EXPECT(present);

    EOKAS_EXPECT(vfs.writeText("/v.txt", "hi") == VfsResult::Ok);
    raw->getPrefix = restChunk(vfsHash("hi", 2));
    raw->getTimes = 1;
    raw->getResult = VfsResult::Io;
    std::vector<u32_t> bad;
    EOKAS_EXPECT(vfs.verify("/v.txt", bad) == VfsResult::Mismatch);
    EOKAS_EXPECT(bad.size() == 1 && bad[0] == 0);
    raw->getTimes = 0;
    EOKAS_EXPECT(vfs.verify("/v.txt", bad) == VfsResult::Ok);
    EOKAS_EXPECT(bad.empty());

    std::vector<u8_t> split(VfsChunkBytes + 1, 'a');
    EOKAS_EXPECT(vfs.writeAll("/split.bin", split.data(), split.size()) == VfsResult::Ok);
    EOKAS_EXPECT(vfs.open("/split.bin", VfsMode::ReadWrite, held) == VfsResult::Ok);
    EOKAS_EXPECT(held.seek((i64_t) VfsChunkBytes, VfsSeek::Begin) == VfsResult::Ok);
    EOKAS_EXPECT(held.write("Z", 1, count) == VfsResult::Ok);
    String head = vfsHash(split.data(), VfsChunkBytes);
    EOKAS_EXPECT(raw->inner.remove(restChunk(head)) == VfsResult::Ok);
    EOKAS_EXPECT(held.flush() == VfsResult::NotFound);
    EOKAS_EXPECT(held.isOpen());
    std::vector<u8_t> headBytes(VfsChunkBytes, 'a');
    EOKAS_EXPECT(raw->inner.put(restChunk(head), headBytes.data(), headBytes.size()) == VfsResult::Ok);
    EOKAS_EXPECT(held.flush() == VfsResult::Ok);
    EOKAS_EXPECT(held.close() == VfsResult::Ok);
    std::vector<u8_t> back;
    EOKAS_EXPECT(vfs.readAll("/split.bin", back) == VfsResult::Ok);
    EOKAS_EXPECT(back.size() == split.size());
    EOKAS_EXPECT(back[0] == 'a' && back.back() == 'Z');

    EOKAS_EXPECT(vfs.open("/split.bin", VfsMode::Read, held) == VfsResult::Ok);
    EOKAS_EXPECT(raw->inner.remove(restChunk(head)) == VfsResult::Ok);
    u8_t byte = 0;
    EOKAS_EXPECT(held.read(&byte, 1, count) == VfsResult::NotFound);
    EOKAS_EXPECT(held.close() == VfsResult::Ok);
    EOKAS_EXPECT(raw->inner.put(restChunk(head), headBytes.data(), headBytes.size()) == VfsResult::Ok);
    EOKAS_EXPECT(vfs.readAll("/v.txt", back) == VfsResult::Ok);
    EOKAS_EXPECT(back.size() == 2);

    EOKAS_EXPECT(vfs.createDir("/empty") == VfsResult::Ok);
    raw->removePrefix = "d/";
    raw->removeTimes = 1;
    raw->removeResult = VfsResult::Io;
    EOKAS_EXPECT(vfs.remove("/empty") == VfsResult::Io);
    present = true;
    EOKAS_EXPECT(vfs.exists("/empty", present) == VfsResult::Ok);
    EOKAS_EXPECT(!present);
    EOKAS_EXPECT(vfs.list("/", entries) == VfsResult::Ok);
    EOKAS_EXPECT(restHas(entries, "empty", VfsKind::Dir));
    raw->removeTimes = 0;
    EOKAS_EXPECT(vfs.remove("/empty") == VfsResult::NotFound);
    EOKAS_EXPECT(vfs.list("/", entries) == VfsResult::Ok);
    EOKAS_EXPECT(restHas(entries, "empty", VfsKind::Dir));

    {
        VfsFile local;
        EOKAS_EXPECT(vfs.open("/drop.txt", VfsMode::Write, local) == VfsResult::Ok);
        EOKAS_EXPECT(local.write("no", 2, count) == VfsResult::Ok);
        raw->putPrefix = "c/";
        raw->putTimes = 1;
        raw->putResult = VfsResult::Io;
    }
    present = true;
    EOKAS_EXPECT(vfs.exists("/drop.txt", present) == VfsResult::Ok);
    EOKAS_EXPECT(!present);
    raw->putTimes = 0;
    return 0;
}

EOKAS_TEST_CASE(vfsoverx) {
    auto lowerOwn = std::make_unique<VfsMemoryVolume>();
    auto upperOwn = std::make_unique<VfsMemoryVolume>();
    VfsMemoryVolume* lower = lowerOwn.get();
    VfsMemoryVolume* upper = upperOwn.get();
    Vfs vfs;
    EOKAS_EXPECT(vfs.attach("/", std::move(lowerOwn), true) == VfsResult::Ok);
    EOKAS_EXPECT(vfs.writeText("/keep.txt", "keep") == VfsResult::Ok);
    EOKAS_EXPECT(vfs.writeText("/save/old.txt", "old") == VfsResult::Ok);
    EOKAS_EXPECT(vfs.writeText("/save/sub/x.txt", "x") == VfsResult::Ok);
    EOKAS_EXPECT(vfs.attach("/save", std::move(upperOwn), true) == VfsResult::Ok);

    String text;
    EOKAS_EXPECT(vfs.readText("/keep.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "keep");
    bool present = true;
    EOKAS_EXPECT(vfs.exists("/save/old.txt", present) == VfsResult::Ok);
    EOKAS_EXPECT(!present);
    EOKAS_EXPECT(vfs.exists("/save/sub/x.txt", present) == VfsResult::Ok);
    EOKAS_EXPECT(!present);
    EOKAS_EXPECT(vfs.readText("/save/old.txt", text) == VfsResult::NotFound);
    EOKAS_EXPECT(vfs.writeText("/save/new.txt", "mem") == VfsResult::Ok);
    EOKAS_EXPECT(vfs.writeText("/keep.txt", "root") == VfsResult::Ok);
    EOKAS_EXPECT(lower->exists(restManifest("/save/new.txt"), present) == VfsResult::Ok);
    EOKAS_EXPECT(!present);
    EOKAS_EXPECT(upper->exists(restManifest("/save/new.txt"), present) == VfsResult::Ok);
    EOKAS_EXPECT(present);
    EOKAS_EXPECT(lower->exists(restManifest("/save/old.txt"), present) == VfsResult::Ok);
    EOKAS_EXPECT(present);
    EOKAS_EXPECT(lower->exists(restManifest("/save/sub/x.txt"), present) == VfsResult::Ok);
    EOKAS_EXPECT(present);

    VfsStat stat;
    EOKAS_EXPECT(vfs.stat("/save", stat) == VfsResult::Ok);
    EOKAS_EXPECT(stat.kind == VfsKind::Dir);
    EOKAS_EXPECT(stat.mtime == 0);
    EOKAS_EXPECT(stat.hash == vfsHash(nullptr, 0));
    std::vector<VfsDirEntry> entries;
    EOKAS_EXPECT(vfs.list("/", entries) == VfsResult::Ok);
    EOKAS_EXPECT(restHas(entries, "save", VfsKind::Dir));
    EOKAS_EXPECT(restHas(entries, "keep.txt", VfsKind::File));
    EOKAS_EXPECT(vfs.list("/save", entries) == VfsResult::Ok);
    EOKAS_EXPECT(restHas(entries, "new.txt", VfsKind::File));
    EOKAS_EXPECT(!restHas(entries, "old.txt", VfsKind::File));
    EOKAS_EXPECT(!restHas(entries, "sub", VfsKind::Dir));
    VfsDiff diff;
    EOKAS_EXPECT(vfs.diff("/keep.txt", "/keep.txt", diff) == VfsResult::Ok);
    EOKAS_EXPECT(diff.sameKind && diff.sameHash);
    EOKAS_EXPECT(vfs.remove("/save") == VfsResult::Access);
    VfsFile file;
    EOKAS_EXPECT(vfs.open("/save", VfsMode::Write, file) == VfsResult::Access);
    EOKAS_EXPECT(vfs.createDir("/save") == VfsResult::Ok);

    EOKAS_EXPECT(vfs.detach("/save") == VfsResult::Ok);
    EOKAS_EXPECT(vfs.readText("/save/old.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "old");
    EOKAS_EXPECT(vfs.readText("/save/sub/x.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "x");
    EOKAS_EXPECT(vfs.exists("/save/new.txt", present) == VfsResult::Ok);
    EOKAS_EXPECT(!present);
    EOKAS_EXPECT(vfs.readText("/keep.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "root");
    EOKAS_EXPECT(vfs.list("/save", entries) == VfsResult::Ok);
    EOKAS_EXPECT(restHas(entries, "old.txt", VfsKind::File));
    EOKAS_EXPECT(restHas(entries, "sub", VfsKind::Dir));
    return 0;
}

EOKAS_TEST_CASE(vfsdiskx) {
    String root = restRoot("persist");
    std::vector<u8_t> wide(VfsChunkBytes + 3, 5);
    {
        Vfs vfs;
        EOKAS_EXPECT(vfs.attach("/", std::make_unique<VfsDiskVolume>(root), true) == VfsResult::Ok);
        EOKAS_EXPECT(vfs.writeText("/a.txt", "same") == VfsResult::Ok);
        EOKAS_EXPECT(vfs.writeText("/b.txt", "same") == VfsResult::Ok);
        EOKAS_EXPECT(vfs.createDir("/empty") == VfsResult::Ok);
        EOKAS_EXPECT(vfs.writeAll("/c.bin", wide.data(), wide.size()) == VfsResult::Ok);
    }

    String shared = vfsHash("same", 4);
    {
        VfsDiskVolume disk(root);
        EOKAS_EXPECT(restRef(disk, shared, 2));
    }

    {
        Vfs vfs;
        EOKAS_EXPECT(vfs.attach("/", std::make_unique<VfsDiskVolume>(root), false) == VfsResult::Ok);
        String text;
        EOKAS_EXPECT(vfs.readText("/a.txt", text) == VfsResult::Ok);
        EOKAS_EXPECT(text == "same");
        EOKAS_EXPECT(vfs.readText("/b.txt", text) == VfsResult::Ok);
        EOKAS_EXPECT(text == "same");
        std::vector<u8_t> back;
        EOKAS_EXPECT(vfs.readAll("/c.bin", back) == VfsResult::Ok);
        EOKAS_EXPECT(back == wide);
        std::vector<u32_t> bad;
        EOKAS_EXPECT(vfs.verify("/c.bin", bad) == VfsResult::Ok);
        EOKAS_EXPECT(vfs.verify("/a.txt", bad) == VfsResult::Ok);
        VfsDiff diff;
        EOKAS_EXPECT(vfs.diff("/a.txt", "/b.txt", diff) == VfsResult::Ok);
        EOKAS_EXPECT(diff.sameKind && diff.sameHash);
        VfsStat stat;
        EOKAS_EXPECT(vfs.stat("/a.txt", stat) == VfsResult::Ok);
        EOKAS_EXPECT(stat.kind == VfsKind::File);
        EOKAS_EXPECT(stat.size == 4);
        EOKAS_EXPECT(stat.hash == shared);
        EOKAS_EXPECT(stat.mtime != 0);
        EOKAS_EXPECT(vfs.stat("/empty", stat) == VfsResult::Ok);
        EOKAS_EXPECT(stat.kind == VfsKind::Dir);
        std::vector<VfsDirEntry> entries;
        EOKAS_EXPECT(vfs.list("/", entries) == VfsResult::Ok);
        EOKAS_EXPECT(restHas(entries, "a.txt", VfsKind::File));
        EOKAS_EXPECT(restHas(entries, "b.txt", VfsKind::File));
        EOKAS_EXPECT(restHas(entries, "c.bin", VfsKind::File));
        EOKAS_EXPECT(restHas(entries, "empty", VfsKind::Dir));
        EOKAS_EXPECT(vfs.writeText("/a.txt", "no") == VfsResult::Access);
        EOKAS_EXPECT(vfs.writeAll("/a.txt", "no", 2) == VfsResult::Access);
        EOKAS_EXPECT(vfs.createDir("/new") == VfsResult::Access);
        EOKAS_EXPECT(vfs.remove("/a.txt") == VfsResult::Access);
        EOKAS_EXPECT(vfs.remove("/missing") == VfsResult::NotFound);
        VfsFile file;
        EOKAS_EXPECT(vfs.open("/a.txt", VfsMode::Read, file) == VfsResult::Ok);
        EOKAS_EXPECT(file.close() == VfsResult::Ok);
        EOKAS_EXPECT(vfs.open("/a.txt", VfsMode::Write, file) == VfsResult::Access);
        EOKAS_EXPECT(vfs.open("/a.txt", VfsMode::Append, file) == VfsResult::Access);
        EOKAS_EXPECT(vfs.open("/a.txt", VfsMode::ReadWrite, file) == VfsResult::Access);
        EOKAS_EXPECT(vfs.open("/missing", VfsMode::Read, file) == VfsResult::NotFound);
        EOKAS_EXPECT(vfs.open("/missing", VfsMode::Append, file) == VfsResult::Access);
        EOKAS_EXPECT(vfs.readText("/a.txt", text) == VfsResult::Ok);
        EOKAS_EXPECT(text == "same");
    }

    {
        Vfs vfs;
        EOKAS_EXPECT(vfs.attach("/", std::make_unique<VfsDiskVolume>(root), true) == VfsResult::Ok);
        EOKAS_EXPECT(vfs.remove("/b.txt") == VfsResult::Ok);
        VfsDiskVolume disk(root);
        EOKAS_EXPECT(restRef(disk, shared, 1));
        String text;
        EOKAS_EXPECT(vfs.readText("/a.txt", text) == VfsResult::Ok);
        EOKAS_EXPECT(text == "same");
        EOKAS_EXPECT(vfs.remove("/a.txt") == VfsResult::Ok);
        bool present = true;
        EOKAS_EXPECT(disk.exists(restChunk(shared), present) == VfsResult::Ok);
        EOKAS_EXPECT(!present);
        EOKAS_EXPECT(vfs.remove("/empty") == VfsResult::Ok);
        EOKAS_EXPECT(vfs.remove("/c.bin") == VfsResult::Ok);
        std::vector<String> keys;
        EOKAS_EXPECT(disk.list("c/", keys) == VfsResult::Ok);
        EOKAS_EXPECT(keys.empty());
        EOKAS_EXPECT(disk.list("k/", keys) == VfsResult::Ok);
        EOKAS_EXPECT(keys.empty());
        std::vector<VfsDirEntry> entries;
        EOKAS_EXPECT(vfs.list("/", entries) == VfsResult::Ok);
        EOKAS_EXPECT(entries.empty());
    }

    {
        Vfs vfs;
        EOKAS_EXPECT(vfs.attach("/", std::make_unique<VfsDiskVolume>(root), false) == VfsResult::Ok);
        bool present = true;
        EOKAS_EXPECT(vfs.exists("/a.txt", present) == VfsResult::Ok);
        EOKAS_EXPECT(!present);
        EOKAS_EXPECT(vfs.exists("/empty", present) == VfsResult::Ok);
        EOKAS_EXPECT(!present);
        EOKAS_EXPECT(vfs.exists("/c.bin", present) == VfsResult::Ok);
        EOKAS_EXPECT(!present);
        String missing;
        EOKAS_EXPECT(vfs.readText("/a.txt", missing) == VfsResult::NotFound);
    }
    return 0;
}

#if COVER_SOCKET

static void restStartupSockets()
{
#if _EOKAS_OS == _EOKAS_OS_WIN64 || _EOKAS_OS == _EOKAS_OS_WIN32
    static bool ready = false;
    if (!ready)
    {
        WSADATA data;
        ::WSAStartup(MAKEWORD(2, 2), &data);
        ready = true;
    }
#endif
}

static void restArm(Socket& socket)
{
#if _EOKAS_OS == _EOKAS_OS_WIN64 || _EOKAS_OS == _EOKAS_OS_WIN32
    DWORD timeout = 5000;
    socket.setOpt(SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
#else
    timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    socket.setOpt(SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
#endif
}

static bool restRecvAll(Socket& socket, void* data, u32_t size)
{
    u8_t* cursor = static_cast<u8_t*>(data);
    u32_t got = 0;
    while (got < size)
    {
        u32_t step = socket.recv(cursor + got, size - got);
        if (step == 0 || step > size - got)
            return false;
        got += step;
    }
    return true;
}

static bool restSendAll(Socket& socket, const void* data, u32_t size)
{
    const u8_t* cursor = static_cast<const u8_t*>(data);
    u32_t sent = 0;
    while (sent < size)
    {
        u32_t step = socket.send((void*) (cursor + sent), size - sent);
        if (step == 0 || step > size - sent)
            return false;
        sent += step;
    }
    return true;
}

static bool restReadFrame(Socket& socket)
{
    u8_t header[20];
    if (!restRecvAll(socket, header, 20))
        return false;
    u32_t keyLen = (u32_t) header[12] | ((u32_t) header[13] << 8) | ((u32_t) header[14] << 16) | ((u32_t) header[15] << 24);
    u32_t payloadLen = (u32_t) header[16] | ((u32_t) header[17] << 8) | ((u32_t) header[18] << 16) | ((u32_t) header[19] << 24);
    if (keyLen > (1u << 20) || payloadLen > (1u << 20))
        return false;
    std::vector<u8_t> skip((size_t) keyLen + payloadLen);
    if (!skip.empty() && !restRecvAll(socket, skip.data(), (u32_t) skip.size()))
        return false;
    return true;
}

struct RestReply
{
    u16_t version = 2;
    u16_t status = 0;
    bool badMagic = false;
    std::vector<u8_t> payload;
    int op = 0;
    VfsResult expect = VfsResult::Ok;
    bool checkPresent = false;
    bool present = false;
    bool checkKeys = false;
    std::vector<String> keys;
};

static bool restWriteReply(Socket& socket, const RestReply& reply)
{
    std::vector<u8_t> header;
    u32_t magic = reply.badMagic ? 0u : 0x53465645u;
    header.push_back((u8_t) (magic & 0xff));
    header.push_back((u8_t) ((magic >> 8) & 0xff));
    header.push_back((u8_t) ((magic >> 16) & 0xff));
    header.push_back((u8_t) ((magic >> 24) & 0xff));
    header.push_back((u8_t) (reply.version & 0xff));
    header.push_back((u8_t) ((reply.version >> 8) & 0xff));
    header.push_back(0);
    header.push_back(0);
    header.push_back((u8_t) (reply.status & 0xff));
    header.push_back((u8_t) ((reply.status >> 8) & 0xff));
    header.push_back(0);
    header.push_back(0);
    header.push_back(0);
    header.push_back(0);
    header.push_back(0);
    header.push_back(0);
    u32_t payloadLen = (u32_t) reply.payload.size();
    header.push_back((u8_t) (payloadLen & 0xff));
    header.push_back((u8_t) ((payloadLen >> 8) & 0xff));
    header.push_back((u8_t) ((payloadLen >> 16) & 0xff));
    header.push_back((u8_t) ((payloadLen >> 24) & 0xff));
    if (!restSendAll(socket, header.data(), (u32_t) header.size()))
        return false;
    if (!reply.payload.empty() && !restSendAll(socket, reply.payload.data(), payloadLen))
        return false;
    return true;
}

static bool restHasKey(const std::vector<String>& keys, const char* key)
{
    for (auto& item : keys)
    {
        if (item == key)
            return true;
    }
    return false;
}

#endif

EOKAS_TEST_CASE(vfsnetx) {
#if !COVER_SOCKET
    VfsMemoryVolume backend;
    VfsNetworkServer server(backend);
    EOKAS_EXPECT(server.listen(0) == VfsResult::Unsupported);
    VfsNetworkVolume offline("127.0.0.1", 1);
    bool present = false;
    EOKAS_EXPECT(offline.exists("k", present) == VfsResult::Unsupported);
    return 0;
#else
    restStartupSockets();
    std::vector<RestReply> replies;
    RestReply existsOn;
    existsOn.payload.push_back(1);
    existsOn.checkPresent = true;
    existsOn.present = true;
    replies.push_back(existsOn);
    RestReply existsOff;
    existsOff.payload.push_back(0);
    existsOff.checkPresent = true;
    existsOff.present = false;
    replies.push_back(existsOff);
    RestReply listEmpty;
    listEmpty.op = 1;
    listEmpty.payload = {0, 0, 0, 0};
    listEmpty.checkKeys = true;
    replies.push_back(listEmpty);
    RestReply listOne;
    listOne.op = 1;
    listOne.payload = {1, 0, 0, 0, 2, 0, 'a', 'b'};
    listOne.checkKeys = true;
    listOne.keys.push_back("ab");
    replies.push_back(listOne);
    RestReply badPayload;
    badPayload.payload.push_back(1);
    badPayload.payload.push_back(2);
    badPayload.expect = VfsResult::Corrupt;
    replies.push_back(badPayload);
    RestReply badStatus;
    badStatus.status = 99;
    badStatus.payload.push_back(1);
    badStatus.expect = VfsResult::Corrupt;
    replies.push_back(badStatus);
    RestReply badVersion;
    badVersion.version = 1;
    badVersion.payload.push_back(1);
    badVersion.expect = VfsResult::Unsupported;
    replies.push_back(badVersion);
    RestReply badMagic;
    badMagic.badMagic = true;
    badMagic.payload.push_back(1);
    badMagic.expect = VfsResult::Io;
    replies.push_back(badMagic);
    RestReply badList;
    badList.op = 1;
    badList.payload = {1, 0, 0, 0};
    badList.expect = VfsResult::Corrupt;
    replies.push_back(badList);

    Socket listen;
    EOKAS_EXPECT(listen.open(AF_INET, SOCK_STREAM, IPPROTO_TCP));
    int reuse = 1;
    listen.setOpt(SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    EOKAS_EXPECT(listen.bind(InetAddress("127.0.0.1", 0)));
    EOKAS_EXPECT(listen.listen(8));
    restArm(listen);
    SocketAddress local = listen.local();
    InetAddress bound(*(sockaddr_in*) &local.value);
    u16_t port = bound.port();
    EOKAS_EXPECT(port != 0);

    std::thread raw([&]() {
        for (size_t index = 0; index < replies.size(); ++index)
        {
            Socket client = listen.accept();
            if (!client.isOpen())
                return;
            restArm(client);
            if (!restReadFrame(client) || !restWriteReply(client, replies[index]))
            {
                client.close();
                return;
            }
            client.close();
        }
    });
    RestJoin rawJoin{raw, &listen};
    for (size_t index = 0; index < replies.size(); ++index)
    {
        VfsNetworkVolume client("127.0.0.1", port);
        RestReply& step = replies[index];
        if (step.op == 0)
        {
            bool present = true;
            VfsResult result = client.exists("pre", present);
            EOKAS_EXPECT(result == step.expect);
            if (step.checkPresent)
                EOKAS_EXPECT(present == step.present);
            if (step.expect == VfsResult::Corrupt)
                EOKAS_EXPECT(present);
        }
        else
        {
            std::vector<String> keys;
            keys.push_back("sentinel");
            VfsResult result = client.list("pre", keys);
            EOKAS_EXPECT(result == step.expect);
            if (step.checkKeys)
            {
                EOKAS_EXPECT(keys.size() == step.keys.size());
                for (size_t key = 0; key < step.keys.size(); ++key)
                    EOKAS_EXPECT(keys[key] == step.keys[key]);
            }
        }
    }
    rawJoin.worker.join();

    RestStatus backend;
    VfsNetworkServer server(backend);
    EOKAS_EXPECT(server.listen(0) == VfsResult::Ok);
    std::thread worker([&server]() {
        server.serveOne();
        server.serveOne();
        server.serveOne();
        server.serveOne();
    });
    RestJoin join{worker};

    {
        VfsNetworkVolume client("127.0.0.1", server.port());
        VfsResult codes[] = {
            VfsResult::InvalidPath,
            VfsResult::NotFound,
            VfsResult::AlreadyExists,
            VfsResult::NotEmpty,
            VfsResult::Access,
            VfsResult::Io,
            VfsResult::Corrupt,
            VfsResult::Mismatch,
            VfsResult::Unsupported,
            VfsResult::Closed
        };
        backend.forceGet = true;
        for (VfsResult code : codes)
        {
            backend.getResult = code;
            std::vector<u8_t> out{9};
            EOKAS_EXPECT(client.get("k", out) == code);
        }
        backend.forceGet = false;
        backend.forceExists = true;
        backend.existsResult = VfsResult::Ok;
        backend.existsValue = false;
        bool present = true;
        EOKAS_EXPECT(client.exists("k", present) == VfsResult::Ok);
        EOKAS_EXPECT(!present);
        backend.existsValue = true;
        EOKAS_EXPECT(client.exists("k", present) == VfsResult::Ok);
        EOKAS_EXPECT(present);
        backend.existsResult = VfsResult::Access;
        present = false;
        EOKAS_EXPECT(client.exists("k", present) == VfsResult::Access);
        EOKAS_EXPECT(!present);
        backend.forceExists = false;
        backend.forcePut = true;
        backend.putResult = VfsResult::AlreadyExists;
        EOKAS_EXPECT(client.put("alpha", "v", 1) == VfsResult::AlreadyExists);
        backend.forcePut = false;
        present = true;
        EOKAS_EXPECT(client.exists("alpha", present) == VfsResult::Ok);
        EOKAS_EXPECT(!present);
        backend.forceList = true;
        backend.listResult = VfsResult::Io;
        std::vector<String> keys;
        EOKAS_EXPECT(client.list("", keys) == VfsResult::Io);
        backend.forceList = false;
        backend.forceRemove = true;
        backend.removeResult = VfsResult::NotFound;
        EOKAS_EXPECT(client.remove("alpha") == VfsResult::NotFound);
        backend.forceRemove = false;

        EOKAS_EXPECT(client.put("n/a", "net-bytes", 9) == VfsResult::Ok);
        std::vector<u8_t> out;
        EOKAS_EXPECT(client.get("n/a", out) == VfsResult::Ok);
        EOKAS_EXPECT(out.size() == 9 && std::memcmp(out.data(), "net-bytes", 9) == 0);
        EOKAS_EXPECT(client.exists("n/a", present) == VfsResult::Ok);
        EOKAS_EXPECT(present);
        EOKAS_EXPECT(client.put("n/e", nullptr, 0) == VfsResult::Ok);
        EOKAS_EXPECT(client.get("n/e", out) == VfsResult::Ok);
        EOKAS_EXPECT(out.empty());
        EOKAS_EXPECT(client.list("n/", keys) == VfsResult::Ok);
        EOKAS_EXPECT(restHasKey(keys, "n/a"));
        EOKAS_EXPECT(restHasKey(keys, "n/e"));
        EOKAS_EXPECT(client.list("", keys) == VfsResult::Ok);
        EOKAS_EXPECT(keys.empty());
        EOKAS_EXPECT(client.get("missing", out) == VfsResult::NotFound);
        EOKAS_EXPECT(client.remove("n/a") == VfsResult::Ok);
        EOKAS_EXPECT(client.exists("n/a", present) == VfsResult::Ok);
        EOKAS_EXPECT(!present);
    }

    {
        Vfs vfs;
        EOKAS_EXPECT(vfs.attach("/", std::make_unique<VfsNetworkVolume>("127.0.0.1", server.port()), true) == VfsResult::Ok);
        EOKAS_EXPECT(vfs.writeText("/note.txt", "net") == VfsResult::Ok);
        EOKAS_EXPECT(vfs.writeText("/note2.txt", "net") == VfsResult::Ok);
        EOKAS_EXPECT(vfs.createDir("/box") == VfsResult::Ok);
        EOKAS_EXPECT(vfs.writeText("/box/a.txt", "f") == VfsResult::Ok);
        String text;
        EOKAS_EXPECT(vfs.readText("/note.txt", text) == VfsResult::Ok);
        EOKAS_EXPECT(text == "net");
        EOKAS_EXPECT(vfs.readText("/note2.txt", text) == VfsResult::Ok);
        EOKAS_EXPECT(text == "net");
        std::vector<VfsDirEntry> entries;
        EOKAS_EXPECT(vfs.list("/", entries) == VfsResult::Ok);
        EOKAS_EXPECT(restHas(entries, "note.txt", VfsKind::File));
        EOKAS_EXPECT(restHas(entries, "note2.txt", VfsKind::File));
        EOKAS_EXPECT(restHas(entries, "box", VfsKind::Dir));
        EOKAS_EXPECT(vfs.list("/box", entries) == VfsResult::Ok);
        EOKAS_EXPECT(restHas(entries, "a.txt", VfsKind::File));
        VfsStat stat;
        EOKAS_EXPECT(vfs.stat("/note.txt", stat) == VfsResult::Ok);
        EOKAS_EXPECT(stat.kind == VfsKind::File);
        EOKAS_EXPECT(stat.size == 3);
        EOKAS_EXPECT(stat.hash == vfsHash("net", 3));
        std::vector<u32_t> bad;
        EOKAS_EXPECT(vfs.verify("/note.txt", bad) == VfsResult::Ok);
        VfsDiff diff;
        EOKAS_EXPECT(vfs.diff("/note.txt", "/note2.txt", diff) == VfsResult::Ok);
        EOKAS_EXPECT(diff.sameHash);
        EOKAS_EXPECT(vfs.remove("/note2.txt") == VfsResult::Ok);
        EOKAS_EXPECT(vfs.readText("/note.txt", text) == VfsResult::Ok);
        EOKAS_EXPECT(text == "net");
    }

    {
        Vfs vfs;
        EOKAS_EXPECT(vfs.attach("/", std::make_unique<VfsNetworkVolume>("127.0.0.1", server.port()), true) == VfsResult::Ok);
        String text;
        EOKAS_EXPECT(vfs.readText("/note.txt", text) == VfsResult::Ok);
        EOKAS_EXPECT(text == "net");
        EOKAS_EXPECT(vfs.remove("/note.txt") == VfsResult::Ok);
        EOKAS_EXPECT(vfs.remove("/box/a.txt") == VfsResult::Ok);
        EOKAS_EXPECT(vfs.remove("/box") == VfsResult::Ok);
        bool present = true;
        EOKAS_EXPECT(vfs.exists("/note.txt", present) == VfsResult::Ok);
        EOKAS_EXPECT(!present);
    }

    backend.forcePut = true;
    backend.putResult = VfsResult::Io;
    {
        Vfs vfs;
        EOKAS_EXPECT(vfs.attach("/", std::make_unique<VfsNetworkVolume>("127.0.0.1", server.port()), true) == VfsResult::Ok);
        EOKAS_EXPECT(vfs.writeText("/fail.txt", "no") == VfsResult::Io);
        bool present = true;
        EOKAS_EXPECT(vfs.exists("/fail.txt", present) == VfsResult::Ok);
        EOKAS_EXPECT(!present);
    }
    return 0;
#endif
}
