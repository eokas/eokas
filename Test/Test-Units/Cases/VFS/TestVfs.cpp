#include "Unit.h"
#include "VFS/Header.h"

#include <cstring>
#include <thread>

using namespace eokas;

static std::unique_ptr<VfsVolume> withBlock(std::unique_ptr<VfsVolume> volume, bool writable)
{
    if (volume != nullptr)
    {
        volume->setScheme(std::make_unique<VfsBlockScheme>());
        volume->setWritable(writable);
    }
    return volume;
}

static String testRoot(const char* tag)
{
    return String::format("vfs-unit-%s-%u", tag, Process::getPID());
}

static bool containsName(const std::vector<VfsDirEntry>& entries, const char* name)
{
    for (auto& entry : entries)
    {
        if (entry.name == name)
            return true;
    }
    return false;
}

static bool containsKind(const std::vector<VfsDirEntry>& entries, const char* name, VfsKind kind)
{
    for (auto& entry : entries)
    {
        if (entry.name == name)
            return entry.kind == kind;
    }
    return false;
}

static bool hasKey(const std::vector<String>& keys, const char* key)
{
    for (auto& item : keys)
    {
        if (item == key)
            return true;
    }
    return false;
}

static int keyCount(VfsVolume& volume, const char* prefix)
{
    std::vector<String> keys;
    if (volume.list(prefix, keys) != VfsResult::Ok)
        return -1;
    return (int) keys.size();
}

EOKAS_TEST_CASE(vfspath) {
    String out;
    EOKAS_EXPECT(VfsPath::normalize("", out) == VfsResult::InvalidPath);
    EOKAS_EXPECT(VfsPath::normalize("a", out) == VfsResult::InvalidPath);
    EOKAS_EXPECT(VfsPath::normalize("\\a", out) == VfsResult::InvalidPath);
    EOKAS_EXPECT(VfsPath::normalize("/a\\b", out) == VfsResult::InvalidPath);
    EOKAS_EXPECT(VfsPath::normalize("C:/a", out) == VfsResult::InvalidPath);
    EOKAS_EXPECT(VfsPath::normalize("mem:/a", out) == VfsResult::InvalidPath);
    EOKAS_EXPECT(VfsPath::normalize("//a", out) == VfsResult::InvalidPath);
    EOKAS_EXPECT(VfsPath::normalize("/.", out) == VfsResult::InvalidPath);
    EOKAS_EXPECT(VfsPath::normalize("/..", out) == VfsResult::InvalidPath);
    EOKAS_EXPECT(VfsPath::normalize("/a/../b", out) == VfsResult::InvalidPath);
    EOKAS_EXPECT(VfsPath::normalize("/a/./b", out) == VfsResult::InvalidPath);
    EOKAS_EXPECT(VfsPath::normalize("/a//b", out) == VfsResult::InvalidPath);
    EOKAS_EXPECT(VfsPath::normalize("/", out) == VfsResult::Ok);
    EOKAS_EXPECT(out == "/");
    EOKAS_EXPECT(VfsPath::normalize("/a/b/", out) == VfsResult::Ok);
    EOKAS_EXPECT(out == "/a/b");
    EOKAS_EXPECT(VfsPath::normalize("/a/b", out) == VfsResult::Ok);
    EOKAS_EXPECT(out == "/a/b");
    EOKAS_EXPECT(VfsPath::parent("/") == "/");
    EOKAS_EXPECT(VfsPath::parent("") == "/");
    EOKAS_EXPECT(VfsPath::parent("/a") == "/");
    EOKAS_EXPECT(VfsPath::parent("/a/b") == "/a");
    EOKAS_EXPECT(VfsPath::name("/").isEmpty());
    EOKAS_EXPECT(VfsPath::name("/a/b") == "b");
    EOKAS_EXPECT(VfsPath::join("/", "a") == "/a");
    EOKAS_EXPECT(VfsPath::join("/a", "b") == "/a/b");
    return 0;
}

EOKAS_TEST_CASE(vfsvol) {
    VfsMemoryVolume memory;
    bool present = true;
    std::vector<u8_t> bytes;
    EOKAS_EXPECT(memory.exists("missing", present) == VfsResult::Ok);
    EOKAS_EXPECT(!present);
    EOKAS_EXPECT(memory.get("missing", bytes) == VfsResult::NotFound);
    EOKAS_EXPECT(memory.remove("missing") == VfsResult::NotFound);
    EOKAS_EXPECT(memory.put("ab/cd", "hi", 2) == VfsResult::Ok);
    EOKAS_EXPECT(memory.put("ab/cd", nullptr, 0) == VfsResult::Ok);
    EOKAS_EXPECT(memory.get("ab/cd", bytes) == VfsResult::Ok);
    EOKAS_EXPECT(bytes.empty());
    EOKAS_EXPECT(memory.put("ab/ef", "x", 1) == VfsResult::Ok);
    EOKAS_EXPECT(memory.put("zz", "z", 1) == VfsResult::Ok);
    std::vector<String> keys;
    EOKAS_EXPECT(memory.list("ab/", keys) == VfsResult::Ok);
    EOKAS_EXPECT(keys.size() == 2);
    EOKAS_EXPECT(hasKey(keys, "ab/cd"));
    EOKAS_EXPECT(hasKey(keys, "ab/ef"));
    EOKAS_EXPECT(memory.remove("ab/cd") == VfsResult::Ok);
    EOKAS_EXPECT(memory.exists("ab/cd", present) == VfsResult::Ok);
    EOKAS_EXPECT(!present);

    String root = testRoot("vol");
    VfsDiskVolume disk(root);
    EOKAS_EXPECT(disk.put("", "a", 1) == VfsResult::InvalidPath);
    EOKAS_EXPECT(disk.put("..", "a", 1) == VfsResult::InvalidPath);
    EOKAS_EXPECT(disk.put("a/../b", "a", 1) == VfsResult::InvalidPath);
    EOKAS_EXPECT(disk.put("/ab", "a", 1) == VfsResult::InvalidPath);
    EOKAS_EXPECT(disk.put("ab/", "a", 1) == VfsResult::InvalidPath);
    EOKAS_EXPECT(disk.put("Ab", "a", 1) == VfsResult::InvalidPath);
    EOKAS_EXPECT(disk.exists("missing", present) == VfsResult::Ok);
    EOKAS_EXPECT(!present);
    EOKAS_EXPECT(disk.get("missing", bytes) == VfsResult::NotFound);
    EOKAS_EXPECT(disk.remove("missing") == VfsResult::NotFound);
    std::vector<u8_t> stored;
    EOKAS_EXPECT(disk.put("ab/cd", "disk", 4) == VfsResult::Ok);
    EOKAS_EXPECT(disk.exists("ab", present) == VfsResult::Ok);
    EOKAS_EXPECT(!present);
    EOKAS_EXPECT(disk.get("ab/cd", stored) == VfsResult::Ok);
    EOKAS_EXPECT(stored.size() == 4 && std::memcmp(stored.data(), "disk", 4) == 0);
    EOKAS_EXPECT(disk.put("ab/cd", nullptr, 0) == VfsResult::Ok);
    EOKAS_EXPECT(disk.get("ab/cd", stored) == VfsResult::Ok);
    EOKAS_EXPECT(stored.empty());
    EOKAS_EXPECT(disk.list("ab", keys) == VfsResult::Ok);
    EOKAS_EXPECT(hasKey(keys, "ab/cd"));
    EOKAS_EXPECT(disk.remove("ab/cd") == VfsResult::Ok);
    EOKAS_EXPECT(disk.exists("ab/cd", present) == VfsResult::Ok);
    EOKAS_EXPECT(!present);

    String nested = testRoot("nested") + "\\parent\\leaf";
    VfsDiskVolume deep(nested);
    EOKAS_EXPECT(deep.put("ab/cd", "nest", 4) == VfsResult::Ok);
    EOKAS_EXPECT(deep.get("ab/cd", stored) == VfsResult::Ok);
    EOKAS_EXPECT(stored.size() == 4 && std::memcmp(stored.data(), "nest", 4) == 0);
    EOKAS_EXPECT(deep.put("ab/cd", "next", 4) == VfsResult::Ok);
    EOKAS_EXPECT(deep.get("ab/cd", stored) == VfsResult::Ok);
    EOKAS_EXPECT(stored.size() == 4 && std::memcmp(stored.data(), "next", 4) == 0);
    EOKAS_EXPECT(deep.list("ab/", keys) == VfsResult::Ok);
    EOKAS_EXPECT(hasKey(keys, "ab/cd"));
    EOKAS_EXPECT(!hasKey(keys, "ab/cd.tmp"));
    EOKAS_EXPECT(File::isFolder(testRoot("nested") + "/parent/leaf"));
    return 0;
}

EOKAS_TEST_CASE(vfsmem) {
    VfsRoot vfsRoot;
    EOKAS_EXPECT(vfsRoot.mount("/", withBlock(std::make_unique<VfsMemoryVolume>(), true)) == VfsResult::Ok);
    EOKAS_EXPECT(vfsRoot.writeText("/t.txt", "hello") == VfsResult::Ok);
    std::vector<u8_t> bytes;
    EOKAS_EXPECT(vfsRoot.readAll("/t.txt", bytes) == VfsResult::Ok);
    EOKAS_EXPECT(bytes.size() == 5);
    EOKAS_EXPECT(std::memcmp(bytes.data(), "hello", 5) == 0);

    u8_t raw[] = {'a', 0, 'b'};
    EOKAS_EXPECT(vfsRoot.writeAll("/b.bin", raw, 3) == VfsResult::Ok);
    std::vector<u8_t> back;
    EOKAS_EXPECT(vfsRoot.readAll("/b.bin", back) == VfsResult::Ok);
    EOKAS_EXPECT(back.size() == 3);
    EOKAS_EXPECT(back[0] == 'a' && back[1] == 0 && back[2] == 'b');

    String text;
    EOKAS_EXPECT(vfsRoot.readText("/t.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "hello");

    VfsStat stat;
    EOKAS_EXPECT(vfsRoot.stat("/t.txt", stat) == VfsResult::Ok);
    EOKAS_EXPECT(stat.kind == VfsKind::File);
    EOKAS_EXPECT(stat.size == 5);
    EOKAS_EXPECT(stat.mtime != 0);
    EOKAS_EXPECT(vfsRoot.stat("/", stat) == VfsResult::Ok);
    EOKAS_EXPECT(stat.kind == VfsKind::Dir);
    EOKAS_EXPECT(stat.size == 0);
    EOKAS_EXPECT(stat.mtime != 0);
    return 0;
}

EOKAS_TEST_CASE(vfsdisk) {
    String root = testRoot("disk");
    {
        VfsRoot vfsRoot;
        EOKAS_EXPECT(vfsRoot.mount("/", withBlock(std::make_unique<VfsDiskVolume>(root), true)) == VfsResult::Ok);
        EOKAS_EXPECT(vfsRoot.writeText("/note.txt", "disk") == VfsResult::Ok);
    }
    VfsRoot again;
    EOKAS_EXPECT(again.mount("/", withBlock(std::make_unique<VfsDiskVolume>(root), false)) == VfsResult::Ok);
    String text;
    EOKAS_EXPECT(again.readText("/note.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "disk");
    EOKAS_EXPECT(again.writeText("/note.txt", "no") == VfsResult::Access);
    return 0;
}

EOKAS_TEST_CASE(vfsio) {
    VfsRoot vfsRoot;
    EOKAS_EXPECT(vfsRoot.mount("/", withBlock(std::make_unique<VfsMemoryVolume>(), true)) == VfsResult::Ok);
    std::vector<u8_t> body(65536 + 4, 7);
    body[65536] = 9;
    body[65536 + 1] = 9;
    body[65536 + 2] = 9;
    body[65536 + 3] = 9;
    EOKAS_EXPECT(vfsRoot.writeAll("/span.bin", body.data(), body.size()) == VfsResult::Ok);

    VfsFile closed;
    u64_t count = 1;
    EOKAS_EXPECT(closed.read(nullptr, 0, count) == VfsResult::Closed);
    EOKAS_EXPECT(!closed.isOpen());
    EOKAS_EXPECT(closed.tell() == 0);
    EOKAS_EXPECT(closed.size() == 0);
    EOKAS_EXPECT(closed.close() == VfsResult::Ok);

    VfsFile file;
    EOKAS_EXPECT(vfsRoot.open("/missing", VfsMode::Read, file) == VfsResult::NotFound);
    EOKAS_EXPECT(vfsRoot.open("/", VfsMode::Read, file) == VfsResult::Access);
    EOKAS_EXPECT(vfsRoot.open("/span.bin", VfsMode::Read, file) == VfsResult::Ok);
    EOKAS_EXPECT(file.isOpen());
    EOKAS_EXPECT(file.size() == body.size());
    EOKAS_EXPECT(file.write("x", 1, count) == VfsResult::Access);
    EOKAS_EXPECT(file.read(nullptr, 0, count) == VfsResult::Ok);
    EOKAS_EXPECT(count == 0);
    EOKAS_EXPECT(file.seek((i64_t) 65536 - 2, VfsSeek::Begin) == VfsResult::Ok);
    u8_t span[16];
    EOKAS_EXPECT(file.read(span, 6, count) == VfsResult::Ok);
    EOKAS_EXPECT(count == 6);
    EOKAS_EXPECT(span[0] == 7 && span[1] == 7 && span[2] == 9 && span[5] == 9);
    EOKAS_EXPECT(file.tell() == 65536 + 4);
    EOKAS_EXPECT(file.seek(-1, VfsSeek::End) == VfsResult::Ok);
    u8_t last = 0;
    EOKAS_EXPECT(file.read(&last, 1, count) == VfsResult::Ok);
    EOKAS_EXPECT(last == 9 && count == 1);
    EOKAS_EXPECT(file.seek(10, VfsSeek::Current) == VfsResult::Ok);
    EOKAS_EXPECT(file.read(&last, 1, count) == VfsResult::Ok);
    EOKAS_EXPECT(count == 0);
    u64_t saved = file.tell();
    EOKAS_EXPECT(file.seek(-1, VfsSeek::Begin) == VfsResult::InvalidPath);
    EOKAS_EXPECT(file.tell() == saved);
    EOKAS_EXPECT(file.seek(3, VfsSeek::Begin) == VfsResult::Ok);
    EOKAS_EXPECT(file.read(span, 10, count) == VfsResult::Ok);
    EOKAS_EXPECT(count == 10);
    String text;
    EOKAS_EXPECT(file.seek(4, VfsSeek::Begin) == VfsResult::Ok);
    EOKAS_EXPECT(file.readText(text) == VfsResult::Ok);
    EOKAS_EXPECT(file.tell() == 4);
    EOKAS_EXPECT(file.close() == VfsResult::Ok);
    EOKAS_EXPECT(file.read(&last, 1, count) == VfsResult::Closed);
    EOKAS_EXPECT(file.close() == VfsResult::Ok);

    VfsFile writer;
    EOKAS_EXPECT(vfsRoot.open("/pad.bin", VfsMode::Write, writer) == VfsResult::Ok);
    EOKAS_EXPECT(writer.seek(2, VfsSeek::Begin) == VfsResult::Ok);
    EOKAS_EXPECT(writer.write("Z", 1, count) == VfsResult::Ok);
    EOKAS_EXPECT(writer.flush() == VfsResult::Ok);
    EOKAS_EXPECT(writer.flush() == VfsResult::Ok);
    EOKAS_EXPECT(writer.close() == VfsResult::Ok);
    std::vector<u8_t> padded;
    EOKAS_EXPECT(vfsRoot.readAll("/pad.bin", padded) == VfsResult::Ok);
    EOKAS_EXPECT(padded.size() == 3);
    EOKAS_EXPECT(padded[0] == 0 && padded[1] == 0 && padded[2] == 'Z');

    EOKAS_EXPECT(vfsRoot.writeText("/edit.txt", "abcd") == VfsResult::Ok);
    VfsStat before;
    EOKAS_EXPECT(vfsRoot.stat("/edit.txt", before) == VfsResult::Ok);
    VfsFile edit;
    EOKAS_EXPECT(vfsRoot.open("/edit.txt", VfsMode::ReadWrite, edit) == VfsResult::Ok);
    EOKAS_EXPECT(edit.close() == VfsResult::Ok);
    VfsStat after;
    EOKAS_EXPECT(vfsRoot.stat("/edit.txt", after) == VfsResult::Ok);
    EOKAS_EXPECT(after.size == before.size);
    EOKAS_EXPECT(after.mtime == before.mtime);
    EOKAS_EXPECT(vfsRoot.open("/edit.txt", VfsMode::ReadWrite, edit) == VfsResult::Ok);
    EOKAS_EXPECT(edit.seek(1, VfsSeek::Begin) == VfsResult::Ok);
    EOKAS_EXPECT(edit.write("XY", 2, count) == VfsResult::Ok);
    EOKAS_EXPECT(edit.close() == VfsResult::Ok);
    EOKAS_EXPECT(vfsRoot.readText("/edit.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "aXYd");

    EOKAS_EXPECT(vfsRoot.open("/edit.txt", VfsMode::Append, edit) == VfsResult::Ok);
    EOKAS_EXPECT(edit.tell() == 4);
    EOKAS_EXPECT(edit.write("!", 1, count) == VfsResult::Ok);
    EOKAS_EXPECT(edit.close() == VfsResult::Ok);
    EOKAS_EXPECT(vfsRoot.readText("/edit.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "aXYd!");

    EOKAS_EXPECT(vfsRoot.open("/edit.txt", VfsMode::ReadWrite, edit) == VfsResult::Ok);
    EOKAS_EXPECT(edit.writeText("zz") == VfsResult::Ok);
    EOKAS_EXPECT(edit.close() == VfsResult::Ok);
    EOKAS_EXPECT(vfsRoot.readText("/edit.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "zzYd!");

    EOKAS_EXPECT(vfsRoot.open("/edit.txt", VfsMode::Append, edit) == VfsResult::Ok);
    EOKAS_EXPECT(edit.writeText("?") == VfsResult::Ok);
    EOKAS_EXPECT(edit.close() == VfsResult::Ok);
    EOKAS_EXPECT(vfsRoot.readText("/edit.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "zzYd!?");

    u8_t rawNul[] = {'a', 0, 'b'};
    EOKAS_EXPECT(vfsRoot.writeAll("/nul.bin", rawNul, 3) == VfsResult::Ok);
    EOKAS_EXPECT(vfsRoot.readText("/nul.bin", text) == VfsResult::Corrupt);
    std::vector<u8_t> nulBytes;
    EOKAS_EXPECT(vfsRoot.readAll("/nul.bin", nulBytes) == VfsResult::Ok);
    EOKAS_EXPECT(nulBytes.size() == 3 && nulBytes[0] == 'a' && nulBytes[1] == 0 && nulBytes[2] == 'b');

    VfsFile fresh;
    EOKAS_EXPECT(vfsRoot.open("/new.bin", VfsMode::ReadWrite, fresh) == VfsResult::Ok);
    EOKAS_EXPECT(fresh.close() == VfsResult::Ok);
    bool exists = false;
    EOKAS_EXPECT(vfsRoot.exists("/new.bin", exists) == VfsResult::Ok);
    EOKAS_EXPECT(exists);
    EOKAS_EXPECT(vfsRoot.open("/new.bin", VfsMode::Read, fresh) == VfsResult::Ok);
    VfsFile moved(std::move(fresh));
    EOKAS_EXPECT(!fresh.isOpen());
    EOKAS_EXPECT(moved.size() == 0);
    EOKAS_EXPECT(moved.close() == VfsResult::Ok);
    return 0;
}

namespace
{
    class PutGateVolume : public VfsVolume
    {
    public:
        bool rejectPut = false;
        int failManifestPuts = 0;
        String failPutKey;
        int failPutKeyTimes = 0;
        String failRemovePrefix;
        int failRemoveTimes = 0;

        VfsResult exists(const String& key, bool& out) override
        {
            return inner.exists(key, out);
        }

        VfsResult get(const String& key, std::vector<u8_t>& out) override
        {
            return inner.get(key, out);
        }

        VfsResult put(const String& key, const void* data, u64_t size) override
        {
            if (rejectPut)
                return VfsResult::Io;
            if (failManifestPuts > 0 && key.startsWith(String("m/")))
            {
                failManifestPuts -= 1;
                return VfsResult::Io;
            }
            if (failPutKeyTimes > 0 && key == failPutKey)
            {
                failPutKeyTimes -= 1;
                return VfsResult::Io;
            }
            return inner.put(key, data, size);
        }

        VfsResult remove(const String& key) override
        {
            if (failRemoveTimes > 0 && !failRemovePrefix.isEmpty() && key.startsWith(failRemovePrefix))
            {
                failRemoveTimes -= 1;
                return VfsResult::Io;
            }
            return inner.remove(key);
        }

        VfsResult list(const String& prefix, std::vector<String>& out) override
        {
            return inner.list(prefix, out);
        }

    private:
        VfsMemoryVolume inner;
    };
}

EOKAS_TEST_CASE(vfshold) {
    auto gate = std::make_unique<PutGateVolume>();
    PutGateVolume* raw = gate.get();
    VfsRoot vfsRoot;
    EOKAS_EXPECT(vfsRoot.mount("/", withBlock(std::move(gate), true)) == VfsResult::Ok);

    VfsFile file;
    EOKAS_EXPECT(file.close() == VfsResult::Ok);
    EOKAS_EXPECT(vfsRoot.open("/hold.txt", VfsMode::Write, file) == VfsResult::Ok);
    u64_t count = 0;
    EOKAS_EXPECT(file.write("hi", 2, count) == VfsResult::Ok);
    raw->rejectPut = true;
    EOKAS_EXPECT(file.flush() == VfsResult::Io);
    EOKAS_EXPECT(file.isOpen());
    EOKAS_EXPECT(file.size() == 2);
    raw->rejectPut = false;
    EOKAS_EXPECT(file.flush() == VfsResult::Ok);
    EOKAS_EXPECT(file.isOpen());
    EOKAS_EXPECT(file.close() == VfsResult::Ok);
    EOKAS_EXPECT(!file.isOpen());
    String text;
    EOKAS_EXPECT(vfsRoot.readText("/hold.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "hi");

    EOKAS_EXPECT(vfsRoot.open("/hold.txt", VfsMode::ReadWrite, file) == VfsResult::Ok);
    EOKAS_EXPECT(file.write("Z", 1, count) == VfsResult::Ok);
    raw->rejectPut = true;
    EOKAS_EXPECT(vfsRoot.open("/other.txt", VfsMode::Write, file) == VfsResult::Io);
    EOKAS_EXPECT(file.isOpen());
    EOKAS_EXPECT(file.size() == 2);
    EOKAS_EXPECT(file.seek(0, VfsSeek::Begin) == VfsResult::Ok);
    char kept[2] = {};
    EOKAS_EXPECT(file.read(kept, 2, count) == VfsResult::Ok);
    EOKAS_EXPECT(count == 2);
    EOKAS_EXPECT(kept[0] == 'Z' && kept[1] == 'i');
    raw->rejectPut = false;
    EOKAS_EXPECT(file.flush() == VfsResult::Ok);
    EOKAS_EXPECT(file.close() == VfsResult::Ok);
    EOKAS_EXPECT(vfsRoot.readText("/hold.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "Zi");
    bool exists = true;
    EOKAS_EXPECT(vfsRoot.exists("/other.txt", exists) == VfsResult::Ok);
    EOKAS_EXPECT(!exists);
    return 0;
}

EOKAS_TEST_CASE(vfsretry) {
    auto gate = std::make_unique<PutGateVolume>();
    PutGateVolume* raw = gate.get();
    VfsRoot vfsRoot;
    EOKAS_EXPECT(vfsRoot.mount("/", withBlock(std::move(gate), true)) == VfsResult::Ok);
    EOKAS_EXPECT(vfsRoot.writeText("/a.txt", "hi") == VfsResult::Ok);
    EOKAS_EXPECT(vfsRoot.writeText("/b.txt", "hi") == VfsResult::Ok);

    VfsFile file;
    u64_t count = 0;
    EOKAS_EXPECT(vfsRoot.open("/a.txt", VfsMode::ReadWrite, file) == VfsResult::Ok);
    EOKAS_EXPECT(file.write("yo", 2, count) == VfsResult::Ok);
    raw->failPutKey = "b/2/0";
    raw->failPutKeyTimes = 1;
    EOKAS_EXPECT(file.flush() == VfsResult::Io);
    String text;
    EOKAS_EXPECT(vfsRoot.readText("/a.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "hi");
    EOKAS_EXPECT(vfsRoot.readText("/b.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "hi");
    EOKAS_EXPECT(file.flush() == VfsResult::Ok);
    EOKAS_EXPECT(file.close() == VfsResult::Ok);
    EOKAS_EXPECT(vfsRoot.readText("/a.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "yo");

    raw->failPutKey = "s/super";
    raw->failPutKeyTimes = 1;
    EOKAS_EXPECT(vfsRoot.remove("/b.txt") == VfsResult::Io);
    EOKAS_EXPECT(vfsRoot.readText("/b.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "hi");
    EOKAS_EXPECT(vfsRoot.remove("/b.txt") == VfsResult::Ok);
    bool exists = true;
    EOKAS_EXPECT(vfsRoot.exists("/b.txt", exists) == VfsResult::Ok);
    EOKAS_EXPECT(!exists);
    EOKAS_EXPECT(vfsRoot.readText("/a.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "yo");
    return 0;
}

EOKAS_TEST_CASE(vfsdir) {
    VfsRoot vfsRoot;
    EOKAS_EXPECT(vfsRoot.mount("/", withBlock(std::make_unique<VfsMemoryVolume>(), true)) == VfsResult::Ok);
    bool exists = false;
    EOKAS_EXPECT(vfsRoot.exists("/", exists) == VfsResult::Ok);
    EOKAS_EXPECT(exists);
    EOKAS_EXPECT(vfsRoot.exists("/missing", exists) == VfsResult::Ok);
    EOKAS_EXPECT(!exists);
    EOKAS_EXPECT(vfsRoot.exists("relative", exists) == VfsResult::InvalidPath);
    EOKAS_EXPECT(vfsRoot.exists("/a/../b", exists) == VfsResult::InvalidPath);
    VfsStat stat;
    EOKAS_EXPECT(vfsRoot.stat("/missing", stat) == VfsResult::NotFound);
    std::vector<VfsDirEntry> entries;
    EOKAS_EXPECT(vfsRoot.list("/", entries) == VfsResult::Ok);
    EOKAS_EXPECT(entries.empty());
    EOKAS_EXPECT(vfsRoot.list("/missing", entries) == VfsResult::NotFound);
    EOKAS_EXPECT(vfsRoot.createDir("/") == VfsResult::Ok);
    EOKAS_EXPECT(vfsRoot.remove("/") == VfsResult::Access);
    EOKAS_EXPECT(vfsRoot.createDir("/box/inner") == VfsResult::Ok);
    EOKAS_EXPECT(vfsRoot.createDir("/box/inner") == VfsResult::Ok);
    EOKAS_EXPECT(vfsRoot.list("/", entries) == VfsResult::Ok);
    EOKAS_EXPECT(containsKind(entries, "box", VfsKind::Dir));
    EOKAS_EXPECT(vfsRoot.list("/box", entries) == VfsResult::Ok);
    EOKAS_EXPECT(containsKind(entries, "inner", VfsKind::Dir));
    EOKAS_EXPECT(vfsRoot.writeText("/box/inner/a.txt", "file") == VfsResult::Ok);
    EOKAS_EXPECT(vfsRoot.list("/box/inner", entries) == VfsResult::Ok);
    EOKAS_EXPECT(containsKind(entries, "a.txt", VfsKind::File));
    EOKAS_EXPECT(vfsRoot.list("/box/inner/a.txt", entries) == VfsResult::Access);
    VfsFile file;
    EOKAS_EXPECT(vfsRoot.open("/box", VfsMode::Read, file) == VfsResult::Access);
    EOKAS_EXPECT(vfsRoot.remove("/box") == VfsResult::NotEmpty);
    EOKAS_EXPECT(vfsRoot.exists("/box", exists) == VfsResult::Ok);
    EOKAS_EXPECT(exists);
    EOKAS_EXPECT(vfsRoot.remove("/box/inner/a.txt") == VfsResult::Ok);
    EOKAS_EXPECT(vfsRoot.list("/box/inner", entries) == VfsResult::Ok);
    EOKAS_EXPECT(entries.empty());
    EOKAS_EXPECT(vfsRoot.remove("/box/inner") == VfsResult::Ok);
    EOKAS_EXPECT(vfsRoot.remove("/box") == VfsResult::Ok);
    EOKAS_EXPECT(vfsRoot.exists("/box", exists) == VfsResult::Ok);
    EOKAS_EXPECT(!exists);

    EOKAS_EXPECT(vfsRoot.writeText("/a.txt", "file") == VfsResult::Ok);
    EOKAS_EXPECT(vfsRoot.createDir("/a.txt") == VfsResult::AlreadyExists);
    EOKAS_EXPECT(vfsRoot.createDir("/a.txt/child") == VfsResult::AlreadyExists);
    EOKAS_EXPECT(vfsRoot.writeAll("/empty", nullptr, 0) == VfsResult::Ok);
    EOKAS_EXPECT(vfsRoot.createDir("/empty-dir") == VfsResult::Ok);
    VfsStat fileStat;
    VfsStat dirStat;
    EOKAS_EXPECT(vfsRoot.stat("/empty", fileStat) == VfsResult::Ok);
    EOKAS_EXPECT(vfsRoot.stat("/empty-dir", dirStat) == VfsResult::Ok);
    EOKAS_EXPECT(fileStat.kind == VfsKind::File);
    EOKAS_EXPECT(dirStat.kind == VfsKind::Dir);
    EOKAS_EXPECT(fileStat.size == 0 && dirStat.size == 0);
    return 0;
}

EOKAS_TEST_CASE(vfsover) {
    String baseRoot = testRoot("base");
    {
        VfsRoot prep;
        EOKAS_EXPECT(prep.mount("/", withBlock(std::make_unique<VfsDiskVolume>(baseRoot), true)) == VfsResult::Ok);
        EOKAS_EXPECT(prep.writeText("/keep.txt", "keep") == VfsResult::Ok);
        EOKAS_EXPECT(prep.writeText("/save/old.txt", "old") == VfsResult::Ok);
    }

    auto save = std::make_unique<VfsMemoryVolume>();
    VfsMemoryVolume* saveRaw = save.get();
    VfsRoot vfsRoot;
    EOKAS_EXPECT(vfsRoot.mount("/", withBlock(std::make_unique<VfsDiskVolume>(baseRoot), true)) == VfsResult::Ok);
    EOKAS_EXPECT(vfsRoot.mount("/save", withBlock(std::move(save), true)) == VfsResult::Ok);

    String text;
    EOKAS_EXPECT(vfsRoot.readText("/keep.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "keep");
    bool present = true;
    EOKAS_EXPECT(vfsRoot.exists("/save/old.txt", present) == VfsResult::Ok);
    EOKAS_EXPECT(!present);
    EOKAS_EXPECT(vfsRoot.writeText("/save/slot.dat", "mem") == VfsResult::Ok);
    EOKAS_EXPECT(vfsRoot.writeText("/keep.txt", "root") == VfsResult::Ok);
    EOKAS_EXPECT(vfsRoot.readText("/save/slot.dat", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "mem");
    EOKAS_EXPECT(vfsRoot.readText("/keep.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "root");
    EOKAS_EXPECT(keyCount(*saveRaw, "b/") > 0);

    VfsStat stat;
    EOKAS_EXPECT(vfsRoot.stat("/save", stat) == VfsResult::Ok);
    EOKAS_EXPECT(stat.kind == VfsKind::Dir);
    EOKAS_EXPECT(stat.size == 0);
    EOKAS_EXPECT(vfsRoot.createDir("/save") == VfsResult::Ok);
    EOKAS_EXPECT(vfsRoot.remove("/save") == VfsResult::Access);
    VfsFile file;
    EOKAS_EXPECT(vfsRoot.open("/save", VfsMode::Read, file) == VfsResult::Access);

    std::vector<VfsDirEntry> entries;
    EOKAS_EXPECT(vfsRoot.list("/", entries) == VfsResult::Ok);
    EOKAS_EXPECT(containsKind(entries, "save", VfsKind::Dir));
    EOKAS_EXPECT(containsKind(entries, "keep.txt", VfsKind::File));
    EOKAS_EXPECT(vfsRoot.list("/save", entries) == VfsResult::Ok);
    EOKAS_EXPECT(containsKind(entries, "slot.dat", VfsKind::File));
    EOKAS_EXPECT(!containsName(entries, "old.txt"));

    EOKAS_EXPECT(vfsRoot.unmount("/save") == VfsResult::Ok);
    EOKAS_EXPECT(vfsRoot.readText("/save/old.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "old");
    EOKAS_EXPECT(vfsRoot.readText("/keep.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "root");
    return 0;
}

EOKAS_TEST_CASE(vfsatt) {
    VfsRoot bare;
    bool exists = false;
    std::vector<VfsDirEntry> entries;
    EOKAS_EXPECT(bare.exists("/", exists) == VfsResult::Ok);
    EOKAS_EXPECT(exists);
    EOKAS_EXPECT(bare.list("/", entries) == VfsResult::Ok);
    EOKAS_EXPECT(entries.empty());
    EOKAS_EXPECT(bare.writeText("/a.txt", "no") == VfsResult::NotFound);
    EOKAS_EXPECT(bare.mount("", withBlock(std::make_unique<VfsMemoryVolume>(), true)) == VfsResult::InvalidPath);
    EOKAS_EXPECT(bare.mount("mem", withBlock(std::make_unique<VfsMemoryVolume>(), true)) == VfsResult::InvalidPath);
    EOKAS_EXPECT(bare.mount("/", withBlock(std::unique_ptr<VfsVolume>(), true)) == VfsResult::Io);
    EOKAS_EXPECT(bare.mount("/", std::make_unique<VfsMemoryVolume>()) == VfsResult::Io);
    EOKAS_EXPECT(bare.mount("/", withBlock(std::make_unique<VfsMemoryVolume>(), false)) == VfsResult::NotFound);
    EOKAS_EXPECT(bare.mount("/", withBlock(std::make_unique<VfsMemoryVolume>(), true)) == VfsResult::Ok);
    EOKAS_EXPECT(bare.mount("/", withBlock(std::make_unique<VfsMemoryVolume>(), true)) == VfsResult::AlreadyExists);
    EOKAS_EXPECT(bare.unmount("/missing") == VfsResult::NotFound);
    EOKAS_EXPECT(bare.unmount("/") == VfsResult::Ok);

    auto root = std::make_unique<VfsMemoryVolume>();
    auto save = std::make_unique<VfsMemoryVolume>();
    auto assets = std::make_unique<VfsMemoryVolume>();
    auto ui = std::make_unique<VfsMemoryVolume>();
    VfsMemoryVolume* rootRaw = root.get();
    VfsMemoryVolume* saveRaw = save.get();
    VfsMemoryVolume* assetsRaw = assets.get();
    VfsMemoryVolume* uiRaw = ui.get();
    VfsRoot vfsRoot;
    EOKAS_EXPECT(vfsRoot.mount("/", withBlock(std::move(root), true)) == VfsResult::Ok);
    EOKAS_EXPECT(vfsRoot.mount("/save", withBlock(std::move(save), true)) == VfsResult::Ok);
    EOKAS_EXPECT(vfsRoot.mount("/assets", withBlock(std::move(assets), true)) == VfsResult::Ok);
    EOKAS_EXPECT(vfsRoot.mount("/assets/ui", withBlock(std::move(ui), true)) == VfsResult::Ok);
    EOKAS_EXPECT(vfsRoot.mount("/assets/ui", withBlock(std::make_unique<VfsMemoryVolume>(), true)) == VfsResult::AlreadyExists);

    EOKAS_EXPECT(vfsRoot.writeText("/save2/a.txt", "root-file") == VfsResult::Ok);
    EOKAS_EXPECT(vfsRoot.writeText("/save/a.txt", "save-file") == VfsResult::Ok);
    EOKAS_EXPECT(vfsRoot.writeText("/assets/b.txt", "asset") == VfsResult::Ok);
    EOKAS_EXPECT(vfsRoot.writeText("/assets/ui/a.txt", "ui") == VfsResult::Ok);
    String text;
    EOKAS_EXPECT(vfsRoot.readText("/save2/a.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "root-file");
    EOKAS_EXPECT(vfsRoot.readText("/save/a.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "save-file");
    EOKAS_EXPECT(vfsRoot.readText("/assets/b.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "asset");
    EOKAS_EXPECT(vfsRoot.readText("/assets/ui/a.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "ui");

    bool present = false;
    EOKAS_EXPECT(rootRaw->exists("i/3", present) == VfsResult::Ok);
    EOKAS_EXPECT(present);
    EOKAS_EXPECT(rootRaw->exists("b/3/0", present) == VfsResult::Ok);
    EOKAS_EXPECT(present);
    EOKAS_EXPECT(saveRaw->exists("i/3", present) == VfsResult::Ok);
    EOKAS_EXPECT(!present);
    EOKAS_EXPECT(saveRaw->exists("b/2/0", present) == VfsResult::Ok);
    EOKAS_EXPECT(present);
    EOKAS_EXPECT(assetsRaw->exists("b/2/0", present) == VfsResult::Ok);
    EOKAS_EXPECT(present);
    EOKAS_EXPECT(assetsRaw->exists("i/3", present) == VfsResult::Ok);
    EOKAS_EXPECT(!present);
    EOKAS_EXPECT(uiRaw->exists("b/2/0", present) == VfsResult::Ok);
    EOKAS_EXPECT(present);

    EOKAS_EXPECT(vfsRoot.list("/assets", entries) == VfsResult::Ok);
    EOKAS_EXPECT(containsKind(entries, "ui", VfsKind::Dir));
    EOKAS_EXPECT(containsKind(entries, "b.txt", VfsKind::File));
    EOKAS_EXPECT(vfsRoot.list("/assets/ui", entries) == VfsResult::Ok);
    EOKAS_EXPECT(containsKind(entries, "a.txt", VfsKind::File));

    EOKAS_EXPECT(vfsRoot.remove("/assets/b.txt") == VfsResult::Ok);
    EOKAS_EXPECT(vfsRoot.remove("/assets") == VfsResult::Access);
    EOKAS_EXPECT(vfsRoot.remove("/assets/ui") == VfsResult::Access);
    EOKAS_EXPECT(vfsRoot.createDir("/box") == VfsResult::Ok);
    EOKAS_EXPECT(vfsRoot.mount("/box/ui", withBlock(std::make_unique<VfsMemoryVolume>(), true)) == VfsResult::Ok);
    EOKAS_EXPECT(vfsRoot.remove("/box") == VfsResult::NotEmpty);
    EOKAS_EXPECT(vfsRoot.unmount("/box/ui") == VfsResult::Ok);
    EOKAS_EXPECT(vfsRoot.remove("/box") == VfsResult::Ok);
    EOKAS_EXPECT(vfsRoot.unmount("/assets/ui") == VfsResult::Ok);

    EOKAS_EXPECT(vfsRoot.writeText("/blocked", "file") == VfsResult::Ok);
    EOKAS_EXPECT(vfsRoot.mount("/blocked", withBlock(std::make_unique<VfsMemoryVolume>(), true)) == VfsResult::AlreadyExists);
    EOKAS_EXPECT(vfsRoot.mount("/blocked/child", withBlock(std::make_unique<VfsMemoryVolume>(), true)) == VfsResult::AlreadyExists);

    auto data = std::make_unique<VfsMemoryVolume>();
    {
        auto scratch = std::make_unique<VfsMemoryVolume>();
        VfsMemoryVolume* raw = scratch.get();
        scratch->setScheme(std::make_unique<VfsBlockScheme>());
        scratch->setWritable(true);
        VfsRoot formatter;
        EOKAS_EXPECT(formatter.mount("/", std::move(scratch)) == VfsResult::Ok);
        const char* letters = "0123456789abcdefghijklmnopqrstuvwxyz";
        for (size_t index = 0; letters[index] != '\0'; ++index)
        {
            std::vector<String> keys;
            EOKAS_EXPECT(raw->list(String(letters[index]), keys) == VfsResult::Ok);
            for (auto& key : keys)
            {
                std::vector<u8_t> bytes;
                EOKAS_EXPECT(raw->get(key, bytes) == VfsResult::Ok);
                EOKAS_EXPECT(data->put(key, bytes.empty() ? nullptr : bytes.data(), bytes.size()) == VfsResult::Ok);
            }
        }
    }
    EOKAS_EXPECT(vfsRoot.mount("/data", withBlock(std::move(data), false)) == VfsResult::Ok);
    EOKAS_EXPECT(vfsRoot.writeText("/data/a.txt", "no") == VfsResult::Access);
    EOKAS_EXPECT(vfsRoot.remove("/data") == VfsResult::Access);
    EOKAS_EXPECT(vfsRoot.readText("/data/a.txt", text) == VfsResult::NotFound);
    return 0;
}

EOKAS_TEST_CASE(vfsnet) {
    VfsMemoryVolume backend;
    VfsNetworkServer bare(backend);
    VfsResult early = bare.serveOne();
    EOKAS_EXPECT(early == VfsResult::Io || early == VfsResult::Unsupported);

    VfsNetworkServer server(backend);
    if (server.listen(0) != VfsResult::Ok)
    {
        VfsNetworkVolume offline("127.0.0.1", 1);
        bool present = false;
        VfsResult result = offline.exists("k", present);
        EOKAS_EXPECT(result == VfsResult::Io || result == VfsResult::Unsupported);
        return 0;
    }

    std::thread worker([&server]() {
        server.serveOne();
        server.serveOne();
    });

    int failed = 0;
    {
        VfsNetworkVolume client("127.0.0.1", server.port());
        const char* message = "net-bytes";
        if (client.put("alpha", message, 9) != VfsResult::Ok)
            failed = 1;
        else
        {
            std::vector<u8_t> out;
            if (client.get("alpha", out) != VfsResult::Ok || out.size() != 9 || std::memcmp(out.data(), message, 9) != 0)
                failed = 1;
            bool present = false;
            if (client.exists("alpha", present) != VfsResult::Ok || !present)
                failed = 1;
            if (client.put("alpha", "next", 4) != VfsResult::Ok)
                failed = 1;
            if (client.get("alpha", out) != VfsResult::Ok || out.size() != 4)
                failed = 1;
            if (client.put("empty", nullptr, 0) != VfsResult::Ok)
                failed = 1;
            if (client.get("empty", out) != VfsResult::Ok || !out.empty())
                failed = 1;
            if (client.put("ab/cd", "p", 1) != VfsResult::Ok)
                failed = 1;
            if (client.put("ab/ef", "q", 1) != VfsResult::Ok)
                failed = 1;
            std::vector<String> keys;
            if (client.list("ab/", keys) != VfsResult::Ok || !hasKey(keys, "ab/cd") || !hasKey(keys, "ab/ef"))
                failed = 1;
            if (client.get("missing", out) != VfsResult::NotFound)
                failed = 1;
            if (client.exists("missing", present) != VfsResult::Ok || present)
                failed = 1;
            if (client.remove("missing") != VfsResult::NotFound)
                failed = 1;
            if (client.remove("alpha") != VfsResult::Ok)
                failed = 1;
            if (client.exists("alpha", present) != VfsResult::Ok || present)
                failed = 1;
            std::vector<u8_t> big((1u << 20) + 1, 1);
            if (client.put("big", big.data(), big.size()) != VfsResult::Ok)
                failed = 1;
            if (client.get("big", out) != VfsResult::Ok || out != big)
                failed = 1;
            if (client.put("huge", "x", (64ull << 20) + 1) != VfsResult::Io)
                failed = 1;
            if (client.put("after", "z", 1) != VfsResult::Ok)
                failed = 1;
            vfsNetworkBreakSocket(client);
            if (client.put("again", "no", 2) != VfsResult::Io)
                failed = 1;
            if (client.put("again", "ok", 2) != VfsResult::Ok)
                failed = 1;
            if (client.get("again", out) != VfsResult::Ok || out.size() != 2 || std::memcmp(out.data(), "ok", 2) != 0)
                failed = 1;
        }
    }
    worker.join();
    return failed;
}

EOKAS_TEST_CASE(vfslife) {
    VfsFile kept;
    {
        VfsRoot vfsRoot;
        EOKAS_EXPECT(vfsRoot.mount("/", withBlock(std::make_unique<VfsMemoryVolume>(), true)) == VfsResult::Ok);
        EOKAS_EXPECT(vfsRoot.writeText("/a.txt", "hi") == VfsResult::Ok);
        EOKAS_EXPECT(vfsRoot.open("/a.txt", VfsMode::Read, kept) == VfsResult::Ok);
    }
    String text;
    EOKAS_EXPECT(kept.readText(text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "hi");
    EOKAS_EXPECT(kept.close() == VfsResult::Ok);

    auto memory = std::make_unique<VfsMemoryVolume>();
    VfsMemoryVolume* raw = memory.get();
    VfsRoot vfsRoot;
    EOKAS_EXPECT(vfsRoot.mount("/", withBlock(std::move(memory), true)) == VfsResult::Ok);
    EOKAS_EXPECT(vfsRoot.writeText("/a.txt", "hi") == VfsResult::Ok);
    VfsFile file;
    EOKAS_EXPECT(vfsRoot.open("/a.txt", VfsMode::ReadWrite, file) == VfsResult::Ok);
    EOKAS_EXPECT(vfsRoot.unmount("/") == VfsResult::Ok);
    bool present = true;
    EOKAS_EXPECT(vfsRoot.exists("/a.txt", present) == VfsResult::Ok);
    EOKAS_EXPECT(!present);
    EOKAS_EXPECT(file.readText(text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "hi");
    EOKAS_EXPECT(file.writeText("yo") == VfsResult::Ok);
    EOKAS_EXPECT(file.flush() == VfsResult::Ok);
    EOKAS_EXPECT(file.readText(text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "yo");
    EOKAS_EXPECT(raw->exists("i/2", present) == VfsResult::Ok);
    EOKAS_EXPECT(present);
    EOKAS_EXPECT(raw->exists("b/2/0", present) == VfsResult::Ok);
    EOKAS_EXPECT(present);
    EOKAS_EXPECT(file.close() == VfsResult::Ok);
    EOKAS_EXPECT(vfsRoot.open("/a.txt", VfsMode::Read, file) == VfsResult::NotFound);
    return 0;
}

EOKAS_TEST_CASE(vfsstream) {
    auto memory = std::make_unique<VfsMemoryVolume>();
    VfsMemoryVolume* raw = memory.get();
    VfsRoot vfsRoot;
    EOKAS_EXPECT(vfsRoot.mount("/", withBlock(std::move(memory), true)) == VfsResult::Ok);

    std::vector<u8_t> body(65536 * 2, 'a');
    body[65536] = 'b';
    EOKAS_EXPECT(vfsRoot.writeAll("/wide.bin", body.data(), body.size()) == VfsResult::Ok);
    EOKAS_EXPECT(keyCount(*raw, "b/") == 2);

    VfsFile file;
    u64_t count = 0;
    EOKAS_EXPECT(vfsRoot.open("/wide.bin", VfsMode::ReadWrite, file) == VfsResult::Ok);
    EOKAS_EXPECT(file.write("Z", 1, count) == VfsResult::Ok);
    EOKAS_EXPECT(file.seek(0, VfsSeek::Begin) == VfsResult::Ok);
    char head[2] = {};
    EOKAS_EXPECT(file.read(head, 2, count) == VfsResult::Ok);
    EOKAS_EXPECT(head[0] == 'Z' && head[1] == 'a');
    EOKAS_EXPECT(file.seek((i64_t) 65536, VfsSeek::Begin) == VfsResult::Ok);
    char mid = 0;
    EOKAS_EXPECT(file.read(&mid, 1, count) == VfsResult::Ok);
    EOKAS_EXPECT(mid == 'b');
    EOKAS_EXPECT(file.close() == VfsResult::Ok);

    std::vector<u8_t> out;
    EOKAS_EXPECT(vfsRoot.readAll("/wide.bin", out) == VfsResult::Ok);
    EOKAS_EXPECT(out.size() == body.size());
    EOKAS_EXPECT(out[0] == 'Z' && out[1] == 'a' && out[65536] == 'b');
    VfsStat wideStat;
    EOKAS_EXPECT(vfsRoot.stat("/wide.bin", wideStat) == VfsResult::Ok);
    EOKAS_EXPECT(wideStat.size == out.size());

    VfsFile gap;
    EOKAS_EXPECT(vfsRoot.open("/gap.bin", VfsMode::Write, gap) == VfsResult::Ok);
    EOKAS_EXPECT(gap.seek((i64_t) 65536 * 2, VfsSeek::Begin) == VfsResult::Ok);
    EOKAS_EXPECT(gap.write("Q", 1, count) == VfsResult::Ok);
    EOKAS_EXPECT(gap.close() == VfsResult::Ok);
    EOKAS_EXPECT(vfsRoot.readAll("/gap.bin", out) == VfsResult::Ok);
    EOKAS_EXPECT(out.size() == 65536 * 2 + 1);
    EOKAS_EXPECT(out[0] == 0 && out[65536] == 0 && out.back() == 'Q');
    VfsStat gapStat;
    EOKAS_EXPECT(vfsRoot.stat("/gap.bin", gapStat) == VfsResult::Ok);
    EOKAS_EXPECT(gapStat.size == out.size());
    bool hole = true;
    EOKAS_EXPECT(raw->exists("b/3/0", hole) == VfsResult::Ok);
    EOKAS_EXPECT(!hole);
    EOKAS_EXPECT(raw->exists("b/3/1", hole) == VfsResult::Ok);
    EOKAS_EXPECT(!hole);
    EOKAS_EXPECT(raw->exists("b/3/2", hole) == VfsResult::Ok);
    EOKAS_EXPECT(hole);
    return 0;
}

EOKAS_TEST_CASE(vfspathx) {
    String out;
    EOKAS_EXPECT(VfsPath::normalize("/a", out) == VfsResult::Ok);
    EOKAS_EXPECT(out == "/a");
    EOKAS_EXPECT(VfsPath::normalize("/a/", out) == VfsResult::Ok);
    EOKAS_EXPECT(out == "/a");
    EOKAS_EXPECT(VfsPath::normalize("/a/b///", out) == VfsResult::Ok);
    EOKAS_EXPECT(out == "/a/b");
    EOKAS_EXPECT(VfsPath::normalize("/./a", out) == VfsResult::InvalidPath);
    EOKAS_EXPECT(VfsPath::normalize("/a/.", out) == VfsResult::InvalidPath);
    EOKAS_EXPECT(VfsPath::normalize("/a/b/..", out) == VfsResult::InvalidPath);
    EOKAS_EXPECT(VfsPath::normalize("/a/b/../c", out) == VfsResult::InvalidPath);
    EOKAS_EXPECT(VfsPath::normalize("/a/b\\c", out) == VfsResult::InvalidPath);
    EOKAS_EXPECT(VfsPath::normalize(":", out) == VfsResult::InvalidPath);
    EOKAS_EXPECT(VfsPath::normalize("/a:", out) == VfsResult::InvalidPath);
    EOKAS_EXPECT(VfsPath::normalize("//", out) == VfsResult::InvalidPath);
    EOKAS_EXPECT(VfsPath::parent("abc") == "/");
    EOKAS_EXPECT(VfsPath::parent("/a/b/c") == "/a/b");
    EOKAS_EXPECT(VfsPath::name("").isEmpty());
    EOKAS_EXPECT(VfsPath::name("abc") == "abc");
    EOKAS_EXPECT(VfsPath::name("/a") == "a");
    EOKAS_EXPECT(VfsPath::join("/a/b", "c") == "/a/b/c");
    EOKAS_EXPECT(VfsPath::join("/", "a/b") == "/a/b");
    EOKAS_EXPECT(VfsPath::join("/a", "") == "/a/");
    EOKAS_EXPECT(VfsPath::normalize(VfsPath::join("/a", ""), out) == VfsResult::Ok);
    EOKAS_EXPECT(out == "/a");
    return 0;
}

EOKAS_TEST_CASE(vfsvolx) {
    VfsMemoryVolume memory;
    bool present = false;
    std::vector<u8_t> bytes;
    EOKAS_EXPECT(memory.put("ab", "x", 1) == VfsResult::Ok);
    EOKAS_EXPECT(memory.put("abc", "y", 1) == VfsResult::Ok);
    EOKAS_EXPECT(memory.put("b", nullptr, 4) == VfsResult::Ok);
    EOKAS_EXPECT(memory.put("", "root", 4) == VfsResult::Ok);
    EOKAS_EXPECT(memory.exists("ab", present) == VfsResult::Ok);
    EOKAS_EXPECT(present);
    EOKAS_EXPECT(memory.get("b", bytes) == VfsResult::Ok);
    EOKAS_EXPECT(bytes.empty());
    EOKAS_EXPECT(memory.put("ab", "yz", 2) == VfsResult::Ok);
    EOKAS_EXPECT(memory.get("ab", bytes) == VfsResult::Ok);
    EOKAS_EXPECT(bytes.size() == 2 && bytes[0] == 'y' && bytes[1] == 'z');
    u8_t raw[] = {0, 1, 255};
    EOKAS_EXPECT(memory.put("bin", raw, 3) == VfsResult::Ok);
    EOKAS_EXPECT(memory.get("bin", bytes) == VfsResult::Ok);
    EOKAS_EXPECT(bytes.size() == 3 && bytes[1] == 1 && bytes[2] == 255);
    EOKAS_EXPECT(memory.exists("", present) == VfsResult::Ok);
    EOKAS_EXPECT(present);
    EOKAS_EXPECT(memory.get("", bytes) == VfsResult::Ok);
    EOKAS_EXPECT(bytes.size() == 4 && std::memcmp(bytes.data(), "root", 4) == 0);
    std::vector<String> keys;
    EOKAS_EXPECT(memory.list("", keys) == VfsResult::Ok);
    EOKAS_EXPECT(keys.empty());
    EOKAS_EXPECT(memory.list("ab", keys) == VfsResult::Ok);
    EOKAS_EXPECT(keys.size() == 2);
    EOKAS_EXPECT(hasKey(keys, "ab"));
    EOKAS_EXPECT(hasKey(keys, "abc"));
    EOKAS_EXPECT(memory.list("z", keys) == VfsResult::Ok);
    EOKAS_EXPECT(keys.empty());
    EOKAS_EXPECT(memory.remove("abc") == VfsResult::Ok);
    EOKAS_EXPECT(memory.remove("abc") == VfsResult::NotFound);
    EOKAS_EXPECT(memory.list("ab", keys) == VfsResult::Ok);
    EOKAS_EXPECT(keys.size() == 1 && keys[0] == "ab");

    const char* badKeys[] = {
        "", "..", "a/../b", "/ab", "ab/", "Ab", "ab/Cd", "ab//cd",
        "a\\b", "a.b", ".", "a/.", "AB", "a b", "a_b"
    };
    String diskRoot = testRoot("volx");
    VfsDiskVolume disk(diskRoot);
    for (auto* key : badKeys)
    {
        EOKAS_EXPECT(disk.put(key, "a", 1) == VfsResult::InvalidPath);
        EOKAS_EXPECT(disk.get(key, bytes) == VfsResult::InvalidPath);
        EOKAS_EXPECT(disk.exists(key, present) == VfsResult::InvalidPath);
        EOKAS_EXPECT(disk.remove(key) == VfsResult::InvalidPath);
    }
    EOKAS_EXPECT(disk.put("ab/12", "ok", 2) == VfsResult::Ok);
    EOKAS_EXPECT(disk.put("zz/1", raw, 3) == VfsResult::Ok);
    EOKAS_EXPECT(disk.put("ab/12", nullptr, 5) == VfsResult::Ok);
    EOKAS_EXPECT(disk.get("ab/12", bytes) == VfsResult::Ok);
    EOKAS_EXPECT(bytes.empty());
    EOKAS_EXPECT(disk.exists("ab/12", present) == VfsResult::Ok);
    EOKAS_EXPECT(present);
    EOKAS_EXPECT(disk.exists("ab", present) == VfsResult::Ok);
    EOKAS_EXPECT(!present);
    EOKAS_EXPECT(disk.get("ab", bytes) == VfsResult::NotFound);
    EOKAS_EXPECT(disk.get("zz/1", bytes) == VfsResult::Ok);
    EOKAS_EXPECT(bytes.size() == 3 && bytes[0] == 0 && bytes[2] == 255);
    EOKAS_EXPECT(disk.list("", keys) == VfsResult::Ok);
    EOKAS_EXPECT(keys.empty());
    EOKAS_EXPECT(disk.list("ab", keys) == VfsResult::Ok);
    EOKAS_EXPECT(keys.size() == 1 && keys[0] == "ab/12");
    EOKAS_EXPECT(disk.list("zz", keys) == VfsResult::Ok);
    EOKAS_EXPECT(keys.size() == 1 && keys[0] == "zz/1");
    EOKAS_EXPECT(disk.list("no", keys) == VfsResult::Ok);
    EOKAS_EXPECT(keys.empty());
    EOKAS_EXPECT(disk.remove("ab/12") == VfsResult::Ok);
    EOKAS_EXPECT(disk.remove("ab/12") == VfsResult::NotFound);
    EOKAS_EXPECT(disk.exists("ab/12", present) == VfsResult::Ok);
    EOKAS_EXPECT(!present);
    EOKAS_EXPECT(disk.get("missing", bytes) == VfsResult::NotFound);

    String slashed = testRoot("slash") + "\\parent\\";
    VfsDiskVolume deep(slashed);
    EOKAS_EXPECT(deep.put("aa/bb", "nest", 4) == VfsResult::Ok);
    EOKAS_EXPECT(deep.get("aa/bb", bytes) == VfsResult::Ok);
    EOKAS_EXPECT(bytes.size() == 4 && std::memcmp(bytes.data(), "nest", 4) == 0);
    EOKAS_EXPECT(File::isFolder(testRoot("slash") + "/parent"));
    return 0;
}
