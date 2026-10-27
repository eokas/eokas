#include "Unit.h"
#include "VFS/Header.h"

#include <cstring>
#include <vector>

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

static String blockRoot(const char* tag)
{
    return String::format("vfs-block-%s-%u", tag, Process::getPID());
}

static VfsResult mountBlock(VfsRoot& vfsRoot, std::unique_ptr<VfsVolume> volume, bool writable)
{
    return vfsRoot.mount("/", withBlock(std::move(volume), writable));
}

EOKAS_TEST_CASE(vfsblock_basic)
{
    VfsRoot vfsRoot;
    EOKAS_EXPECT(mountBlock(vfsRoot, std::make_unique<VfsMemoryVolume>(), true) == VfsResult::Ok);
    EOKAS_EXPECT(vfsRoot.writeText("/t.txt", "hello") == VfsResult::Ok);
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

    EOKAS_EXPECT(vfsRoot.createDir("/docs/sub") == VfsResult::Ok);
    EOKAS_EXPECT(vfsRoot.writeText("/docs/sub/a.txt", "hi") == VfsResult::Ok);
    std::vector<VfsDirEntry> entries;
    EOKAS_EXPECT(vfsRoot.list("/docs", entries) == VfsResult::Ok);
    EOKAS_EXPECT(entries.size() == 1 && entries[0].name == "sub" && entries[0].kind == VfsKind::Dir);
    EOKAS_EXPECT(vfsRoot.remove("/docs") == VfsResult::NotEmpty);
    EOKAS_EXPECT(vfsRoot.link("/docs", "/docs2") == VfsResult::Access);
    EOKAS_EXPECT(vfsRoot.rename("/docs", "/docs/sub/inside") == VfsResult::Access);

    EOKAS_EXPECT(vfsRoot.link("/t.txt", "/copy.txt") == VfsResult::Ok);
    EOKAS_EXPECT(vfsRoot.remove("/t.txt") == VfsResult::Ok);
    EOKAS_EXPECT(vfsRoot.readText("/copy.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "hello");
    bool exists = true;
    EOKAS_EXPECT(vfsRoot.exists("/t.txt", exists) == VfsResult::Ok);
    EOKAS_EXPECT(!exists);

    EOKAS_EXPECT(vfsRoot.writeText("/old.txt", "aaa") == VfsResult::Ok);
    EOKAS_EXPECT(vfsRoot.writeText("/new.txt", "bbb") == VfsResult::Ok);
    EOKAS_EXPECT(vfsRoot.rename("/old.txt", "/new.txt") == VfsResult::Ok);
    EOKAS_EXPECT(vfsRoot.readText("/new.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "aaa");
    return 0;
}

EOKAS_TEST_CASE(vfsblock_disk)
{
    String root = blockRoot("disk");
    {
        VfsRoot vfsRoot;
        EOKAS_EXPECT(mountBlock(vfsRoot, std::make_unique<VfsDiskVolume>(root), true) == VfsResult::Ok);
        std::vector<u8_t> body(196608, 'a');
        EOKAS_EXPECT(vfsRoot.writeAll("/wide.bin", body.data(), body.size()) == VfsResult::Ok);
        VfsFile file;
        EOKAS_EXPECT(vfsRoot.open("/wide.bin", VfsMode::ReadWrite, file) == VfsResult::Ok);
        EOKAS_EXPECT(file.seek(70000, VfsSeek::Begin) == VfsResult::Ok);
        u64_t count = 0;
        EOKAS_EXPECT(file.write("b", 1, count) == VfsResult::Ok);
        EOKAS_EXPECT(file.close() == VfsResult::Ok);
    }
    {
        VfsRoot vfsRoot;
        EOKAS_EXPECT(mountBlock(vfsRoot, std::make_unique<VfsDiskVolume>(root), true) == VfsResult::Ok);
        std::vector<u8_t> back;
        EOKAS_EXPECT(vfsRoot.readAll("/wide.bin", back) == VfsResult::Ok);
        EOKAS_EXPECT(back.size() == 196608);
        EOKAS_EXPECT(back[0] == 'a' && back[69999] == 'a' && back[70000] == 'b' && back[70001] == 'a');
        EOKAS_EXPECT(File::isFile(File::combinePath(root, "i/1")));
        EOKAS_EXPECT(File::isFile(File::combinePath(root, "b/2/0")));
        EOKAS_EXPECT(File::isFile(File::combinePath(root, "b/2/1")));
        EOKAS_EXPECT(!File::isFolder(File::combinePath(root, "c")));

        VfsFile hole;
        EOKAS_EXPECT(vfsRoot.open("/hole.bin", VfsMode::ReadWrite, hole) == VfsResult::Ok);
        EOKAS_EXPECT(hole.seek(100000, VfsSeek::Begin) == VfsResult::Ok);
        u64_t count = 0;
        EOKAS_EXPECT(hole.write("xyz", 3, count) == VfsResult::Ok);
        EOKAS_EXPECT(hole.close() == VfsResult::Ok);
        std::vector<u8_t> sparse;
        EOKAS_EXPECT(vfsRoot.readAll("/hole.bin", sparse) == VfsResult::Ok);
        EOKAS_EXPECT(sparse.size() == 100003);
        EOKAS_EXPECT(sparse[0] == 0);
        EOKAS_EXPECT(sparse[100000] == 'x' && sparse[100001] == 'y' && sparse[100002] == 'z');
        EOKAS_EXPECT(!File::isFile(File::combinePath(root, "b/3/0")));
        EOKAS_EXPECT(File::isFile(File::combinePath(root, "b/3/1")));
    }
    return 0;
}

EOKAS_TEST_CASE(vfsblock_open)
{
    String root = blockRoot("open");
    {
        VfsRoot vfsRoot;
        EOKAS_EXPECT(mountBlock(vfsRoot, std::make_unique<VfsDiskVolume>(root), true) == VfsResult::Ok);
        EOKAS_EXPECT(vfsRoot.writeText("/a.txt", "hello") == VfsResult::Ok);
        VfsFile file;
        EOKAS_EXPECT(vfsRoot.open("/a.txt", VfsMode::ReadWrite, file) == VfsResult::Ok);
        EOKAS_EXPECT(vfsRoot.rename("/a.txt", "/b.txt") == VfsResult::Ok);
        u64_t count = 0;
        EOKAS_EXPECT(file.write("Y", 1, count) == VfsResult::Ok);
        EOKAS_EXPECT(file.close() == VfsResult::Ok);
        String text;
        EOKAS_EXPECT(vfsRoot.readText("/b.txt", text) == VfsResult::Ok);
        EOKAS_EXPECT(text == "Yello");
        bool exists = true;
        EOKAS_EXPECT(vfsRoot.exists("/a.txt", exists) == VfsResult::Ok);
        EOKAS_EXPECT(!exists);

        EOKAS_EXPECT(vfsRoot.writeText("/gone.txt", "stay") == VfsResult::Ok);
        VfsFile held;
        EOKAS_EXPECT(vfsRoot.open("/gone.txt", VfsMode::Read, held) == VfsResult::Ok);
        EOKAS_EXPECT(vfsRoot.remove("/gone.txt") == VfsResult::Ok);
        EOKAS_EXPECT(vfsRoot.exists("/gone.txt", exists) == VfsResult::Ok);
        EOKAS_EXPECT(!exists);
        char buf[8];
        EOKAS_EXPECT(held.read(buf, 4, count) == VfsResult::Ok);
        EOKAS_EXPECT(count == 4 && std::memcmp(buf, "stay", 4) == 0);
        EOKAS_EXPECT(held.close() == VfsResult::Ok);
    }
    {
        VfsRoot vfsRoot;
        EOKAS_EXPECT(mountBlock(vfsRoot, std::make_unique<VfsDiskVolume>(root), false) == VfsResult::Ok);
        bool exists = true;
        EOKAS_EXPECT(vfsRoot.exists("/gone.txt", exists) == VfsResult::Ok);
        EOKAS_EXPECT(!exists);
        EOKAS_EXPECT(!File::isFile(File::combinePath(root, "i/3")));
        EOKAS_EXPECT(!File::isFile(File::combinePath(root, "b/3/0")));
        EOKAS_EXPECT(vfsRoot.writeText("/z.txt", "no") == VfsResult::Access);
        String text;
        EOKAS_EXPECT(vfsRoot.readText("/b.txt", text) == VfsResult::Ok);
        EOKAS_EXPECT(text == "Yello");
    }
    {
        String empty = blockRoot("empty-ro");
        VfsRoot vfsRoot;
        EOKAS_EXPECT(mountBlock(vfsRoot, std::make_unique<VfsDiskVolume>(empty), false) == VfsResult::NotFound);
    }
    {
        String junk = blockRoot("junk");
        VfsDiskVolume seeded(junk);
        EOKAS_EXPECT(seeded.put("c/ab/dead", "x", 1) == VfsResult::Ok);
        VfsRoot vfsRoot;
        EOKAS_EXPECT(vfsRoot.mount("/", withBlock(std::make_unique<VfsDiskVolume>(junk), true)) == VfsResult::Corrupt);
    }
    return 0;
}
