#include "Unit.h"
#include "VFS/Header.h"

#include <cstring>
#include <thread>
#include <vector>

#if _EOKAS_OS == _EOKAS_OS_WIN64 || _EOKAS_OS == _EOKAS_OS_WIN32
#pragma comment(lib, "ws2_32.lib")
#endif

using namespace eokas;

static String coverRoot(const char* tag)
{
    return String::format("vfs-unit-%s-%u", tag, Process::getPID());
}

static bool coverHas(const std::vector<VfsDirEntry>& entries, const char* name, VfsKind kind)
{
    for (auto& entry : entries)
    {
        if (entry.name == name)
            return entry.kind == kind;
    }
    return false;
}

static bool coverHasKey(const std::vector<String>& keys, const char* key)
{
    for (auto& item : keys)
    {
        if (item == key)
            return true;
    }
    return false;
}

static bool coverRef(VfsVolume& volume, const String& hash, u32_t expect)
{
    std::vector<u8_t> bytes;
    if (volume.get(String("k/") + hash, bytes) != VfsResult::Ok || bytes.size() != 4)
        return false;
    u32_t count = (u32_t) bytes[0] | ((u32_t) bytes[1] << 8) | ((u32_t) bytes[2] << 16) | ((u32_t) bytes[3] << 24);
    return count == expect;
}

static String coverChunkKey(const String& hash)
{
    return String("c/") + hash.substr(0, 2) + "/" + hash;
}

static String coverManifestKey(const String& path)
{
    return String("m/") + VfsPath::hash(path);
}

static String coverDirKey(const String& path)
{
    return String("d/") + VfsPath::hash(path);
}

static VfsChunkRef coverRefOf(const void* data, u32_t size)
{
    VfsChunkRef chunk;
    chunk.hash = vfsHash(data, size);
    chunk.length = size;
    return chunk;
}

static VfsManifest coverFile(const char* path, const void* data, u32_t size)
{
    VfsManifest manifest;
    manifest.kind = VfsKind::File;
    manifest.path = path;
    manifest.size = size;
    manifest.hash = vfsHash(data, size);
    manifest.mtime = 1;
    manifest.chunkSize = VfsChunkBytes;
    if (size > 0)
        manifest.chunks.push_back(coverRefOf(data, size));
    return manifest;
}

static VfsResult coverPlant(VfsVolume& volume, const VfsManifest& manifest)
{
    std::vector<u8_t> bytes;
    VfsResult result = encodeManifest(manifest, bytes);
    if (result != VfsResult::Ok)
        return result;
    return volume.put(coverManifestKey(manifest.path), bytes.data(), bytes.size());
}

static void coverPushU16(std::vector<u8_t>& out, u16_t value)
{
    out.push_back((u8_t) (value & 0xff));
    out.push_back((u8_t) ((value >> 8) & 0xff));
}

static void coverPushU32(std::vector<u8_t>& out, u32_t value)
{
    out.push_back((u8_t) (value & 0xff));
    out.push_back((u8_t) ((value >> 8) & 0xff));
    out.push_back((u8_t) ((value >> 16) & 0xff));
    out.push_back((u8_t) ((value >> 24) & 0xff));
}

namespace
{
    class FaultVolume : public VfsVolume
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

    class StatusVolume : public VfsVolume
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
    EOKAS_EXPECT(VfsPath::hash("/cfg/a.txt") == VfsPath::hash("/cfg/a.txt"));
    EOKAS_EXPECT(VfsPath::hash("/cfg/a.txt") != VfsPath::hash("/cfg/b.txt"));
    EOKAS_EXPECT(VfsPath::hash("/a") == vfsHash("/a", 2));
    return 0;
}

EOKAS_TEST_CASE(vfshashx) {
    char emptyByte = 0;
    String empty = vfsHash(nullptr, 0);
    EOKAS_EXPECT(vfsHash("", 0) == empty);
    EOKAS_EXPECT(vfsHash(&emptyByte, 0) == empty);
    EOKAS_EXPECT(vfsHash("hello", 5) == vfsHash("hello", 5));
    EOKAS_EXPECT(vfsHash("hello", 4) != vfsHash("hello", 5));
    EOKAS_EXPECT(vfsHash("hello", 5) != vfsHash("hellp", 5));
    u8_t raw[] = {0, 1, 2, 255};
    EOKAS_EXPECT(vfsHash(raw, 4).length() == 64);
    EOKAS_EXPECT(vfsHash(raw, 4) != vfsHash(raw, 3));
    EOKAS_EXPECT(SHA256().compute(raw, 4) == vfsHash(raw, 4));
    return 0;
}

EOKAS_TEST_CASE(vfsmanx) {
    VfsManifest dir;
    dir.kind = VfsKind::Dir;
    dir.path = "/box";
    dir.size = 0;
    dir.hash = vfsHash(nullptr, 0);
    dir.mtime = -7;
    dir.chunkSize = VfsChunkBytes;
    std::vector<u8_t> bytes;
    EOKAS_EXPECT(encodeManifest(dir, bytes) == VfsResult::Ok);
    VfsManifest back;
    EOKAS_EXPECT(decodeManifest(bytes.data(), bytes.size(), back) == VfsResult::Ok);
    EOKAS_EXPECT(back.kind == VfsKind::Dir);
    EOKAS_EXPECT(back.path == "/box");
    EOKAS_EXPECT(back.mtime == -7);
    EOKAS_EXPECT(back.chunks.empty());

    VfsManifest tomb;
    tomb.kind = VfsKind::Tombstone;
    tomb.path = "";
    tomb.size = 0;
    tomb.hash = vfsHash(nullptr, 0);
    tomb.chunkSize = 0;
    EOKAS_EXPECT(encodeManifest(tomb, bytes) == VfsResult::Ok);
    EOKAS_EXPECT(decodeManifest(bytes.data(), bytes.size(), back) == VfsResult::Ok);
    EOKAS_EXPECT(back.kind == VfsKind::Tombstone);
    EOKAS_EXPECT(back.path.isEmpty());
    std::vector<u8_t> extra = bytes;
    extra.push_back(9);
    EOKAS_EXPECT(decodeManifest(extra.data(), extra.size(), back) == VfsResult::Ok);

    VfsChunkRef badChunk;
    badChunk.hash = "short";
    badChunk.length = 1;
    dir.chunks.push_back(badChunk);
    EOKAS_EXPECT(encodeManifest(dir, bytes) == VfsResult::Corrupt);
    dir.chunks.clear();
    dir.hash = vfsHash(nullptr, 0);
    dir.path = String('a', 65536);
    EOKAS_EXPECT(encodeManifest(dir, bytes) == VfsResult::InvalidPath);

    VfsManifest file = coverFile("/a", "hi", 2);
    EOKAS_EXPECT(encodeManifest(file, bytes) == VfsResult::Ok);
    std::vector<u8_t> patched = bytes;
    patched[0] = 0;
    EOKAS_EXPECT(decodeManifest(patched.data(), patched.size(), back) == VfsResult::Corrupt);
    patched = bytes;
    patched[6] = 9;
    EOKAS_EXPECT(decodeManifest(patched.data(), patched.size(), back) == VfsResult::Corrupt);
    patched = bytes;
    patched[94] = 0x41;
    patched[95] = 0x42;
    patched[96] = 0x0f;
    patched[97] = 0;
    EOKAS_EXPECT(decodeManifest(patched.data(), patched.size(), back) == VfsResult::Corrupt);
    patched = bytes;
    patched[8] = 0x88;
    patched[9] = 0x13;
    EOKAS_EXPECT(decodeManifest(patched.data(), patched.size(), back) == VfsResult::Corrupt);
    EOKAS_EXPECT(decodeManifest(bytes.data(), 12, back) == VfsResult::Corrupt);
    VfsManifest named = coverFile("/ab", nullptr, 0);
    EOKAS_EXPECT(encodeManifest(named, bytes) == VfsResult::Ok);
    patched = bytes;
    EOKAS_EXPECT(patched.size() > 98);
    patched[98] = 0;
    EOKAS_EXPECT(decodeManifest(patched.data(), patched.size(), back) == VfsResult::Corrupt);

    std::vector<VfsDirEntry> entries;
    EOKAS_EXPECT(encodeDir(entries, bytes) == VfsResult::Ok);
    EOKAS_EXPECT(decodeDir(bytes.data(), bytes.size(), entries) == VfsResult::Ok);
    EOKAS_EXPECT(entries.empty());
    VfsDirEntry blank;
    blank.name = "";
    blank.kind = VfsKind::File;
    VfsDirEntry stone;
    stone.name = "t";
    stone.kind = VfsKind::Tombstone;
    entries.push_back(blank);
    entries.push_back(stone);
    EOKAS_EXPECT(encodeDir(entries, bytes) == VfsResult::Ok);
    std::vector<VfsDirEntry> decoded;
    EOKAS_EXPECT(decodeDir(bytes.data(), bytes.size(), decoded) == VfsResult::Ok);
    EOKAS_EXPECT(decoded.size() == 2);
    EOKAS_EXPECT(decoded[0].name.isEmpty() && decoded[0].kind == VfsKind::File);
    EOKAS_EXPECT(decoded[1].name == "t" && decoded[1].kind == VfsKind::Tombstone);
    extra = bytes;
    extra.push_back(1);
    EOKAS_EXPECT(decodeDir(extra.data(), extra.size(), decoded) == VfsResult::Ok);

    entries[0].name = String('n', 65536);
    EOKAS_EXPECT(encodeDir(entries, bytes) == VfsResult::InvalidPath);
    u8_t hugeCount[] = {0x41, 0x42, 0x0f, 0x00};
    EOKAS_EXPECT(decodeDir(hugeCount, 4, decoded) == VfsResult::Corrupt);
    u8_t shortName[] = {1, 0, 0, 0, 5, 0, 'a'};
    EOKAS_EXPECT(decodeDir(shortName, 7, decoded) == VfsResult::Corrupt);
    u8_t nulName[] = {1, 0, 0, 0, 2, 0, 0, 'b', 1};
    EOKAS_EXPECT(decodeDir(nulName, 9, decoded) == VfsResult::Corrupt);

    VfsManifest left;
    left.kind = VfsKind::File;
    left.size = 9;
    left.hash = vfsHash("left", 4);
    left.chunkSize = 3;
    left.chunks.push_back(coverRefOf("aaa", 3));
    left.chunks.push_back(coverRefOf("bbb", 3));
    left.chunks.push_back(coverRefOf("ccc", 3));
    VfsManifest right = left;
    right.hash = vfsHash("right", 5);
    right.chunks[1] = coverRefOf("xxx", 3);
    VfsDiff mid = diffManifest(left, right);
    EOKAS_EXPECT(mid.sameKind && !mid.sameHash);
    EOKAS_EXPECT(mid.spans.size() == 3);
    EOKAS_EXPECT(mid.spans[0].kind == VfsDiffKind::Equal);
    EOKAS_EXPECT(mid.spans[1].kind == VfsDiffKind::Modified);
    EOKAS_EXPECT(mid.spans[1].leftIndex == 1 && mid.spans[1].rightIndex == 1);
    EOKAS_EXPECT(mid.spans[1].leftHash == left.chunks[1].hash);
    EOKAS_EXPECT(mid.spans[1].rightHash == right.chunks[1].hash);
    EOKAS_EXPECT(mid.spans[2].kind == VfsDiffKind::Equal);

    right = left;
    right.hash = vfsHash("short", 5);
    right.chunks.resize(1);
    right.size = 3;
    VfsDiff leftOnly = diffManifest(left, right);
    EOKAS_EXPECT(leftOnly.spans.size() == 3);
    EOKAS_EXPECT(leftOnly.spans[0].kind == VfsDiffKind::Equal);
    EOKAS_EXPECT(leftOnly.spans[1].kind == VfsDiffKind::LeftOnly);
    EOKAS_EXPECT(leftOnly.spans[1].leftIndex == 1);
    EOKAS_EXPECT(leftOnly.spans[1].rightIndex == 1);
    EOKAS_EXPECT(leftOnly.spans[1].rightLength == 0);
    EOKAS_EXPECT(leftOnly.spans[2].kind == VfsDiffKind::LeftOnly);
    VfsDiff rightOnly = diffManifest(right, left);
    EOKAS_EXPECT(rightOnly.spans[1].kind == VfsDiffKind::RightOnly);
    EOKAS_EXPECT(rightOnly.spans[1].leftLength == 0);
    EOKAS_EXPECT(rightOnly.spans[1].rightIndex == 1);
    EOKAS_EXPECT(rightOnly.spans[2].kind == VfsDiffKind::RightOnly);
    EOKAS_EXPECT(rightOnly.spans[2].rightHash == left.chunks[2].hash);

    right = left;
    right.hash = vfsHash("size", 4);
    right.chunkSize = 9;
    VfsDiff sized = diffManifest(left, right);
    EOKAS_EXPECT(sized.spans.size() == 1);
    EOKAS_EXPECT(sized.spans[0].kind == VfsDiffKind::Modified);
    EOKAS_EXPECT(sized.spans[0].leftLength == 9);
    EOKAS_EXPECT(sized.spans[0].leftHash == left.hash);
    EOKAS_EXPECT(sized.spans[0].rightHash == right.hash);

    right = left;
    right.chunks.clear();
    VfsDiff same = diffManifest(left, right);
    EOKAS_EXPECT(same.sameHash);
    EOKAS_EXPECT(same.spans.size() == 1);
    EOKAS_EXPECT(same.spans[0].kind == VfsDiffKind::Equal);

    VfsManifest dirLeft;
    dirLeft.kind = VfsKind::Dir;
    dirLeft.hash = vfsHash(nullptr, 0);
    dirLeft.chunkSize = VfsChunkBytes;
    VfsManifest dirRight = dirLeft;
    VfsDiff dirs = diffManifest(dirLeft, dirRight);
    EOKAS_EXPECT(dirs.sameKind && dirs.sameHash);
    EOKAS_EXPECT(dirs.spans.size() == 1);
    EOKAS_EXPECT(dirs.spans[0].kind == VfsDiffKind::Equal);
    return 0;
}

EOKAS_TEST_CASE(vfsstore) {
    auto volume = std::make_shared<VfsMemoryVolume>();
    VfsChunkStore store(volume);
    VfsManifest missing;
    EOKAS_EXPECT(store.readManifest("/no", missing) == VfsResult::NotFound);
    EOKAS_EXPECT(store.removeManifest("/no") == VfsResult::Ok);
    std::vector<VfsDirEntry> ignored;
    EOKAS_EXPECT(store.readDir("/no", ignored) == VfsResult::NotFound);
    EOKAS_EXPECT(store.removeDir("/no") == VfsResult::Ok);
    EOKAS_EXPECT(store.removeChild("/no", "a") == VfsResult::Ok);

    VfsManifest empty = coverFile("/empty", nullptr, 0);
    EOKAS_EXPECT(store.writeManifest(empty) == VfsResult::Ok);
    VfsManifest loaded;
    EOKAS_EXPECT(store.readManifest("/empty", loaded) == VfsResult::Ok);
    EOKAS_EXPECT(loaded.kind == VfsKind::File && loaded.size == 0);
    EOKAS_EXPECT(store.removeManifest("/empty") == VfsResult::Ok);
    EOKAS_EXPECT(store.readManifest("/empty", loaded) == VfsResult::NotFound);
    EOKAS_EXPECT(store.removeManifest("/empty") == VfsResult::Ok);

    empty.hash = "bad";
    EOKAS_EXPECT(store.writeManifest(empty) == VfsResult::Corrupt);
    bool present = true;
    EOKAS_EXPECT(volume->exists(coverManifestKey("/empty"), present) == VfsResult::Ok);
    EOKAS_EXPECT(!present);

    std::vector<u8_t> junk{1, 2, 3, 4};
    EOKAS_EXPECT(volume->put(coverManifestKey("/bad"), junk.data(), junk.size()) == VfsResult::Ok);
    EOKAS_EXPECT(store.readManifest("/bad", loaded) == VfsResult::Corrupt);
    VfsManifest swapped = coverFile("/one", "ab", 2);
    EOKAS_EXPECT(coverPlant(*volume, swapped) == VfsResult::Ok);
    std::vector<u8_t> stored;
    EOKAS_EXPECT(volume->get(coverManifestKey("/one"), stored) == VfsResult::Ok);
    EOKAS_EXPECT(volume->put(coverManifestKey("/two"), stored.data(), stored.size()) == VfsResult::Ok);
    EOKAS_EXPECT(store.readManifest("/two", loaded) == VfsResult::Corrupt);

    std::vector<VfsDirEntry> entries;
    VfsDirEntry fileEntry;
    fileEntry.name = "a.txt";
    fileEntry.kind = VfsKind::File;
    entries.push_back(fileEntry);
    EOKAS_EXPECT(store.writeDir("/box", entries) == VfsResult::Ok);
    std::vector<VfsDirEntry> listed;
    EOKAS_EXPECT(store.readDir("/box", listed) == VfsResult::Ok);
    EOKAS_EXPECT(listed.size() == 1 && listed[0].kind == VfsKind::File);
    EOKAS_EXPECT(store.upsertChild("/box", "a.txt", VfsKind::Dir) == VfsResult::Ok);
    EOKAS_EXPECT(store.upsertChild("/box", "b.txt", VfsKind::File) == VfsResult::Ok);
    EOKAS_EXPECT(store.readDir("/box", listed) == VfsResult::Ok);
    EOKAS_EXPECT(listed.size() == 2);
    EOKAS_EXPECT(listed[0].kind == VfsKind::Dir);
    EOKAS_EXPECT(listed[1].name == "b.txt");
    EOKAS_EXPECT(store.upsertChild("/box", "", VfsKind::File) == VfsResult::InvalidPath);
    EOKAS_EXPECT(store.upsertChild("/box", "a/b", VfsKind::File) == VfsResult::InvalidPath);
    EOKAS_EXPECT(store.removeChild("/box", "missing") == VfsResult::Ok);
    EOKAS_EXPECT(store.removeChild("/box", "a.txt") == VfsResult::Ok);
    EOKAS_EXPECT(store.readDir("/box", listed) == VfsResult::Ok);
    EOKAS_EXPECT(listed.size() == 1 && listed[0].name == "b.txt");
    EOKAS_EXPECT(volume->put(coverDirKey("/box"), junk.data(), junk.size()) == VfsResult::Ok);
    EOKAS_EXPECT(store.readDir("/box", listed) == VfsResult::Corrupt);
    EOKAS_EXPECT(store.removeChild("/box", "b.txt") == VfsResult::Corrupt);
    EOKAS_EXPECT(store.upsertChild("/box", "c.txt", VfsKind::File) == VfsResult::Corrupt);
    EOKAS_EXPECT(store.removeDir("/box") == VfsResult::Ok);
    EOKAS_EXPECT(store.readDir("/box", listed) == VfsResult::NotFound);
    EOKAS_EXPECT(store.removeDir("/box") == VfsResult::Ok);

    String hash = vfsHash("ab", 2);
    EOKAS_EXPECT(store.getChunk("short", stored) == VfsResult::Corrupt);
    EOKAS_EXPECT(store.getChunk(hash, stored) == VfsResult::NotFound);
    EOKAS_EXPECT(store.putChunk(hash, "zz", 2) == VfsResult::Ok);
    EOKAS_EXPECT(store.putChunk(hash, "ab", 2) == VfsResult::Ok);
    EOKAS_EXPECT(store.getChunk(hash, stored) == VfsResult::Ok);
    EOKAS_EXPECT(stored.size() == 2 && stored[0] == 'z');
    EOKAS_EXPECT(store.addRefs(hash, 0) == VfsResult::Ok);
    EOKAS_EXPECT(volume->exists(String("k/") + hash, present) == VfsResult::Ok);
    EOKAS_EXPECT(!present);
    EOKAS_EXPECT(store.addRefs(hash, 5) == VfsResult::Ok);
    EOKAS_EXPECT(coverRef(*volume, hash, 5));
    EOKAS_EXPECT(store.releaseRefs(hash, 0) == VfsResult::Ok);
    EOKAS_EXPECT(store.releaseRefs(hash, 2) == VfsResult::Ok);
    EOKAS_EXPECT(coverRef(*volume, hash, 3));
    EOKAS_EXPECT(volume->exists(coverChunkKey(hash), present) == VfsResult::Ok);
    EOKAS_EXPECT(present);
    u8_t maxRef[] = {0xff, 0xff, 0xff, 0xff};
    EOKAS_EXPECT(volume->put(String("k/") + hash, maxRef, 4) == VfsResult::Ok);
    EOKAS_EXPECT(store.addRefs(hash, 1) == VfsResult::Io);
    EOKAS_EXPECT(coverRef(*volume, hash, 0xffffffffu));
    u8_t shortRef[] = {1, 0};
    EOKAS_EXPECT(volume->put(String("k/") + hash, shortRef, 2) == VfsResult::Ok);
    EOKAS_EXPECT(store.addRefs(hash, 1) == VfsResult::Corrupt);
    EOKAS_EXPECT(store.releaseRefs(hash, 1) == VfsResult::Corrupt);
    EOKAS_EXPECT(volume->remove(String("k/") + hash) == VfsResult::Ok);
    EOKAS_EXPECT(store.addRefs(hash, 1) == VfsResult::Ok);
    EOKAS_EXPECT(store.releaseRefs(hash, 1) == VfsResult::Ok);
    EOKAS_EXPECT(volume->exists(coverChunkKey(hash), present) == VfsResult::Ok);
    EOKAS_EXPECT(!present);
    EOKAS_EXPECT(volume->exists(String("k/") + hash, present) == VfsResult::Ok);
    EOKAS_EXPECT(!present);
    EOKAS_EXPECT(store.releaseRefs(hash, 1) == VfsResult::Corrupt);

    VfsManifest good = coverFile("/good", "ab", 2);
    EOKAS_EXPECT(store.putChunk(good.chunks[0].hash, "ab", 2) == VfsResult::Ok);
    std::vector<u32_t> bad;
    EOKAS_EXPECT(store.verify(good, bad) == VfsResult::Ok);
    EOKAS_EXPECT(bad.empty());
    EOKAS_EXPECT(volume->remove(coverChunkKey(good.chunks[0].hash)) == VfsResult::Ok);
    EOKAS_EXPECT(store.verify(good, bad) == VfsResult::Mismatch);
    EOKAS_EXPECT(bad.size() == 1 && bad[0] == 0);
    EOKAS_EXPECT(store.putChunk(good.chunks[0].hash, "a", 1) == VfsResult::Ok);
    EOKAS_EXPECT(store.verify(good, bad) == VfsResult::Mismatch);
    EOKAS_EXPECT(bad.size() == 1 && bad[0] == 0);
    EOKAS_EXPECT(volume->remove(coverChunkKey(good.chunks[0].hash)) == VfsResult::Ok);
    EOKAS_EXPECT(volume->put(coverChunkKey(good.chunks[0].hash), "zz", 2) == VfsResult::Ok);
    EOKAS_EXPECT(store.verify(good, bad) == VfsResult::Mismatch);
    EOKAS_EXPECT(bad.size() == 1 && bad[0] == 0);
    EOKAS_EXPECT(volume->put(coverChunkKey(good.chunks[0].hash), "ab", 2) == VfsResult::Ok);
    good.size = 3;
    EOKAS_EXPECT(store.verify(good, bad) == VfsResult::Mismatch);
    EOKAS_EXPECT(bad.empty());
    good.size = 2;
    good.hash = vfsHash("no", 2);
    EOKAS_EXPECT(store.verify(good, bad) == VfsResult::Mismatch);
    EOKAS_EXPECT(bad.empty());
    good.hash = vfsHash("ab", 2);
    EOKAS_EXPECT(store.verify(good, bad) == VfsResult::Ok);

    VfsManifest lied;
    lied.kind = VfsKind::File;
    lied.path = "/lied";
    lied.size = 0;
    lied.hash = vfsHash("x", 1);
    lied.chunkSize = VfsChunkBytes;
    EOKAS_EXPECT(store.verify(lied, bad) == VfsResult::Mismatch);
    EOKAS_EXPECT(bad.empty());
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
    EOKAS_EXPECT(coverHasKey(keys, "ab"));
    EOKAS_EXPECT(coverHasKey(keys, "abc"));
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
    String diskRoot = coverRoot("volx");
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

    String slashed = coverRoot("slash") + "\\parent\\";
    VfsDiskVolume deep(slashed);
    EOKAS_EXPECT(deep.put("aa/bb", "nest", 4) == VfsResult::Ok);
    EOKAS_EXPECT(deep.get("aa/bb", bytes) == VfsResult::Ok);
    EOKAS_EXPECT(bytes.size() == 4 && std::memcmp(bytes.data(), "nest", 4) == 0);
    EOKAS_EXPECT(File::isFolder(coverRoot("slash") + "/parent"));
    return 0;
}

EOKAS_TEST_CASE(vfschain) {
    auto memory = std::make_unique<VfsMemoryVolume>();
    VfsMemoryVolume* raw = memory.get();
    Vfs vfs;
    EOKAS_EXPECT(vfs.attach("/", std::move(memory), true) == VfsResult::Ok);

    EOKAS_EXPECT(vfs.createDir("/cfg/ui") == VfsResult::Ok);
    EOKAS_EXPECT(vfs.createDir("/cfg/ui") == VfsResult::Ok);
    EOKAS_EXPECT(vfs.writeText("/cfg/ui/a.txt", "alpha") == VfsResult::Ok);
    EOKAS_EXPECT(vfs.writeText("/cfg/ui/b.txt", "alpha") == VfsResult::Ok);
    EOKAS_EXPECT(vfs.writeText("/cfg/ui/c.txt", "alpha") == VfsResult::Ok);
    String shared = vfsHash("alpha", 5);
    EOKAS_EXPECT(coverRef(*raw, shared, 3));

    std::vector<u8_t> wide(VfsChunkBytes * 2 + 4, 'a');
    wide[VfsChunkBytes] = 'b';
    wide[VfsChunkBytes + 1] = 'b';
    EOKAS_EXPECT(vfs.writeAll("/wide.bin", wide.data(), wide.size()) == VfsResult::Ok);
    std::vector<u8_t> edited = wide;
    edited[VfsChunkBytes + 1] = 'c';
    EOKAS_EXPECT(vfs.writeAll("/edit.bin", edited.data(), edited.size()) == VfsResult::Ok);
    VfsDiff spans;
    EOKAS_EXPECT(vfs.diff("/wide.bin", "/edit.bin", spans) == VfsResult::Ok);
    EOKAS_EXPECT(spans.sameKind && !spans.sameHash);
    EOKAS_EXPECT(spans.spans.size() == 3);
    EOKAS_EXPECT(spans.spans[0].kind == VfsDiffKind::Equal);
    EOKAS_EXPECT(spans.spans[1].kind == VfsDiffKind::Modified);
    EOKAS_EXPECT(spans.spans[2].kind == VfsDiffKind::Equal);

    std::vector<u8_t> head(VfsChunkBytes, 'a');
    EOKAS_EXPECT(vfs.writeAll("/head.bin", head.data(), head.size()) == VfsResult::Ok);
    EOKAS_EXPECT(vfs.diff("/wide.bin", "/head.bin", spans) == VfsResult::Ok);
    EOKAS_EXPECT(spans.spans.size() == 3);
    EOKAS_EXPECT(spans.spans[0].kind == VfsDiffKind::Equal);
    EOKAS_EXPECT(spans.spans[1].kind == VfsDiffKind::LeftOnly);
    EOKAS_EXPECT(spans.spans[2].kind == VfsDiffKind::LeftOnly);
    EOKAS_EXPECT(vfs.diff("/head.bin", "/wide.bin", spans) == VfsResult::Ok);
    EOKAS_EXPECT(spans.spans[1].kind == VfsDiffKind::RightOnly);
    EOKAS_EXPECT(spans.spans[2].kind == VfsDiffKind::RightOnly);

    VfsFile file;
    u64_t count = 0;
    EOKAS_EXPECT(vfs.open("/wide.bin", VfsMode::Read, file) == VfsResult::Ok);
    EOKAS_EXPECT(file.seek((i64_t) VfsChunkBytes - 2, VfsSeek::Begin) == VfsResult::Ok);
    u8_t bridge[4];
    EOKAS_EXPECT(file.read(bridge, 4, count) == VfsResult::Ok);
    EOKAS_EXPECT(count == 4);
    EOKAS_EXPECT(bridge[0] == 'a' && bridge[1] == 'a' && bridge[2] == 'b' && bridge[3] == 'b');
    EOKAS_EXPECT(file.seek(0, VfsSeek::Begin) == VfsResult::Ok);
    u8_t again = 0;
    EOKAS_EXPECT(file.read(&again, 1, count) == VfsResult::Ok);
    EOKAS_EXPECT(again == 'a');
    EOKAS_EXPECT(file.seek(2, VfsSeek::Current) == VfsResult::Ok);
    EOKAS_EXPECT(file.tell() == 3);
    EOKAS_EXPECT(file.writeText("no") == VfsResult::Access);
    EOKAS_EXPECT(file.flush() == VfsResult::Ok);
    u64_t saved = file.tell();
    EOKAS_EXPECT(file.seek(-100, VfsSeek::Current) == VfsResult::InvalidPath);
    EOKAS_EXPECT(file.tell() == saved);
    EOKAS_EXPECT(file.seek(-((i64_t) file.size() + 1), VfsSeek::End) == VfsResult::InvalidPath);
    EOKAS_EXPECT(file.tell() == saved);
    EOKAS_EXPECT(file.seek(0, VfsSeek::End) == VfsResult::Ok);
    EOKAS_EXPECT(file.read(&again, 1, count) == VfsResult::Ok);
    EOKAS_EXPECT(count == 0);
    EOKAS_EXPECT(file.seek((i64_t) file.size() + 10, VfsSeek::Begin) == VfsResult::Ok);
    EOKAS_EXPECT(file.read(&again, 1, count) == VfsResult::Ok);
    EOKAS_EXPECT(count == 0);
    EOKAS_EXPECT(file.close() == VfsResult::Ok);

    EOKAS_EXPECT(vfs.open("/cfg/ui/a.txt", VfsMode::Append, file) == VfsResult::Ok);
    EOKAS_EXPECT(file.tell() == 5);
    EOKAS_EXPECT(file.close() == VfsResult::Ok);
    String text;
    EOKAS_EXPECT(vfs.readText("/cfg/ui/a.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "alpha");
    EOKAS_EXPECT(vfs.open("/cfg/ui/a.txt", VfsMode::Append, file) == VfsResult::Ok);
    EOKAS_EXPECT(file.seek(1, VfsSeek::Begin) == VfsResult::Ok);
    EOKAS_EXPECT(file.write("Z", 1, count) == VfsResult::Ok);
    EOKAS_EXPECT(file.close() == VfsResult::Ok);
    EOKAS_EXPECT(vfs.readText("/cfg/ui/a.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "aZpha");
    EOKAS_EXPECT(coverRef(*raw, shared, 2));

    EOKAS_EXPECT(vfs.open("/made.txt", VfsMode::Append, file) == VfsResult::Ok);
    EOKAS_EXPECT(file.tell() == 0);
    EOKAS_EXPECT(file.write("yo", 2, count) == VfsResult::Ok);
    EOKAS_EXPECT(file.close() == VfsResult::Ok);
    EOKAS_EXPECT(vfs.readText("/made.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "yo");

    EOKAS_EXPECT(vfs.writeText("/edit.txt", "abcd") == VfsResult::Ok);
    EOKAS_EXPECT(vfs.open("/edit.txt", VfsMode::ReadWrite, file) == VfsResult::Ok);
    EOKAS_EXPECT(file.seek(0, VfsSeek::End) == VfsResult::Ok);
    EOKAS_EXPECT(file.write("XY", 2, count) == VfsResult::Ok);
    EOKAS_EXPECT(file.close() == VfsResult::Ok);
    EOKAS_EXPECT(vfs.readText("/edit.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "abcdXY");

    EOKAS_EXPECT(vfs.writeText("/wipe.txt", "hello") == VfsResult::Ok);
    String wiped = vfsHash("hello", 5);
    EOKAS_EXPECT(coverRef(*raw, wiped, 1));
    EOKAS_EXPECT(vfs.open("/wipe.txt", VfsMode::Write, file) == VfsResult::Ok);
    EOKAS_EXPECT(file.close() == VfsResult::Ok);
    EOKAS_EXPECT(vfs.readText("/wipe.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text.isEmpty());
    bool present = true;
    EOKAS_EXPECT(raw->exists(coverChunkKey(wiped), present) == VfsResult::Ok);
    EOKAS_EXPECT(!present);

    EOKAS_EXPECT(vfs.writeText("/pad.txt", "ab") == VfsResult::Ok);
    EOKAS_EXPECT(vfs.open("/pad.txt", VfsMode::Write, file) == VfsResult::Ok);
    EOKAS_EXPECT(file.write(nullptr, 0, count) == VfsResult::Ok);
    EOKAS_EXPECT(file.close() == VfsResult::Ok);
    EOKAS_EXPECT(vfs.readText("/pad.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text.isEmpty());
    EOKAS_EXPECT(vfs.writeText("/zero.txt", "abcd") == VfsResult::Ok);
    EOKAS_EXPECT(vfs.open("/zero.txt", VfsMode::ReadWrite, file) == VfsResult::Ok);
    EOKAS_EXPECT(file.seek(1, VfsSeek::Begin) == VfsResult::Ok);
    EOKAS_EXPECT(file.write(nullptr, 2, count) == VfsResult::Ok);
    EOKAS_EXPECT(count == 2);
    EOKAS_EXPECT(file.close() == VfsResult::Ok);
    EOKAS_EXPECT(vfs.readText("/zero.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "abcd");
    EOKAS_EXPECT(vfs.open("/zero.txt", VfsMode::ReadWrite, file) == VfsResult::Ok);
    EOKAS_EXPECT(file.seek(0, VfsSeek::End) == VfsResult::Ok);
    EOKAS_EXPECT(file.write(nullptr, 2, count) == VfsResult::Ok);
    EOKAS_EXPECT(count == 2);
    EOKAS_EXPECT(file.close() == VfsResult::Ok);
    EOKAS_EXPECT(vfs.readText("/zero.txt", text) == VfsResult::Corrupt);

    std::vector<u8_t> zeroBytes;
    EOKAS_EXPECT(vfs.readAll("/zero.txt", zeroBytes) == VfsResult::Ok);
    EOKAS_EXPECT(zeroBytes.size() == 6);
    EOKAS_EXPECT(zeroBytes[0] == 'a' && zeroBytes[1] == 'b' && zeroBytes[2] == 'c' && zeroBytes[3] == 'd');
    EOKAS_EXPECT(zeroBytes[4] == 0 && zeroBytes[5] == 0);

    VfsFile gap;
    EOKAS_EXPECT(vfs.open("/gap.bin", VfsMode::Write, gap) == VfsResult::Ok);
    EOKAS_EXPECT(gap.seek((i64_t) VfsChunkBytes * 2, VfsSeek::Begin) == VfsResult::Ok);
    EOKAS_EXPECT(gap.write("Q", 1, count) == VfsResult::Ok);
    EOKAS_EXPECT(gap.seek(0, VfsSeek::Begin) == VfsResult::Ok);
    std::vector<u8_t> gapBytes((size_t) gap.size(), 1);
    EOKAS_EXPECT(gap.read(gapBytes.data(), gapBytes.size(), count) == VfsResult::Ok);
    EOKAS_EXPECT(count == gapBytes.size());
    EOKAS_EXPECT(gapBytes[0] == 0);
    EOKAS_EXPECT(gapBytes[VfsChunkBytes] == 0);
    EOKAS_EXPECT(gapBytes.back() == 'Q');
    EOKAS_EXPECT(gap.close() == VfsResult::Ok);

    {
        VfsFile local;
        EOKAS_EXPECT(vfs.open("/life.txt", VfsMode::Write, local) == VfsResult::Ok);
        EOKAS_EXPECT(local.write("zz", 2, count) == VfsResult::Ok);
    }
    EOKAS_EXPECT(vfs.readText("/life.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "zz");

    VfsFile moved;
    EOKAS_EXPECT(vfs.open("/life.txt", VfsMode::ReadWrite, moved) == VfsResult::Ok);
    EOKAS_EXPECT(moved.write("Q", 1, count) == VfsResult::Ok);
    VfsFile taken;
    taken = std::move(moved);
    EOKAS_EXPECT(!moved.isOpen());
    EOKAS_EXPECT(taken.isOpen());
    taken = std::move(taken);
    EOKAS_EXPECT(taken.isOpen());
    EOKAS_EXPECT(taken.close() == VfsResult::Ok);
    EOKAS_EXPECT(vfs.readText("/life.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "Qz");

    EOKAS_EXPECT(vfs.remove("/cfg/ui/b.txt") == VfsResult::Ok);
    EOKAS_EXPECT(coverRef(*raw, shared, 1));
    EOKAS_EXPECT(vfs.remove("/cfg/ui/c.txt") == VfsResult::Ok);
    EOKAS_EXPECT(raw->exists(coverChunkKey(shared), present) == VfsResult::Ok);
    EOKAS_EXPECT(!present);
    std::vector<VfsDirEntry> entries;
    EOKAS_EXPECT(vfs.list("/cfg/ui", entries) == VfsResult::Ok);
    EOKAS_EXPECT(coverHas(entries, "a.txt", VfsKind::File));
    EOKAS_EXPECT(!coverHas(entries, "b.txt", VfsKind::File));
    EOKAS_EXPECT(vfs.remove("/cfg/ui/a.txt") == VfsResult::Ok);
    EOKAS_EXPECT(vfs.remove("/cfg/ui") == VfsResult::Ok);
    EOKAS_EXPECT(vfs.remove("/cfg") == VfsResult::Ok);

    VfsStat stat;
    stat.kind = VfsKind::Tombstone;
    stat.size = 9;
    stat.hash = "keep";
    stat.mtime = 3;
    EOKAS_EXPECT(vfs.stat("relative", stat) == VfsResult::InvalidPath);
    EOKAS_EXPECT(stat.kind == VfsKind::Tombstone && stat.size == 9 && stat.hash == "keep");
    std::vector<u32_t> bad;
    EOKAS_EXPECT(vfs.verify("relative", bad) == VfsResult::InvalidPath);
    VfsDiff kept;
    kept.sameKind = false;
    kept.spans.push_back(VfsChunkSpan{});
    EOKAS_EXPECT(vfs.diff("/wide.bin", "relative", kept) == VfsResult::InvalidPath);
    EOKAS_EXPECT(kept.spans.size() == 1);
    EOKAS_EXPECT(vfs.diff("/missing", "/wide.bin", kept) == VfsResult::NotFound);
    EOKAS_EXPECT(vfs.createDir("/d1") == VfsResult::Ok);
    EOKAS_EXPECT(vfs.createDir("/d2") == VfsResult::Ok);
    EOKAS_EXPECT(vfs.diff("/d1", "/d2", spans) == VfsResult::Ok);
    EOKAS_EXPECT(spans.sameKind && spans.sameHash);
    EOKAS_EXPECT(spans.spans.size() == 1);
    EOKAS_EXPECT(spans.spans[0].kind == VfsDiffKind::Equal);

    VfsManifest manifestTomb;
    manifestTomb.kind = VfsKind::Tombstone;
    manifestTomb.path = "/ghost";
    manifestTomb.size = 0;
    manifestTomb.hash = vfsHash(nullptr, 0);
    manifestTomb.chunkSize = VfsChunkBytes;
    EOKAS_EXPECT(coverPlant(*raw, manifestTomb) == VfsResult::Ok);
    present = true;
    EOKAS_EXPECT(vfs.exists("/ghost", present) == VfsResult::Ok);
    EOKAS_EXPECT(!present);
    EOKAS_EXPECT(vfs.stat("/ghost", stat) == VfsResult::NotFound);
    EOKAS_EXPECT(vfs.open("/ghost", VfsMode::Read, file) == VfsResult::NotFound);
    EOKAS_EXPECT(vfs.remove("/ghost") == VfsResult::NotFound);
    EOKAS_EXPECT(vfs.open("/ghost", VfsMode::Write, file) == VfsResult::Ok);
    EOKAS_EXPECT(file.writeText("hi") == VfsResult::Ok);
    EOKAS_EXPECT(file.close() == VfsResult::Ok);
    EOKAS_EXPECT(vfs.readText("/ghost", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "hi");
    EOKAS_EXPECT(vfs.list("/", entries) == VfsResult::Ok);
    EOKAS_EXPECT(coverHas(entries, "ghost", VfsKind::File));

    EOKAS_EXPECT(vfs.writeText("/shown.txt", "show") == VfsResult::Ok);
    EOKAS_EXPECT(vfs.writeText("/hidden.txt", "hide") == VfsResult::Ok);
    std::vector<u8_t> dirBytes;
    EOKAS_EXPECT(raw->get(coverDirKey("/"), dirBytes) == VfsResult::Ok);
    EOKAS_EXPECT(decodeDir(dirBytes.data(), dirBytes.size(), entries) == VfsResult::Ok);
    bool marked = false;
    for (auto& entry : entries)
    {
        if (entry.name == "hidden.txt")
        {
            entry.kind = VfsKind::Tombstone;
            marked = true;
        }
    }
    EOKAS_EXPECT(marked);
    EOKAS_EXPECT(encodeDir(entries, dirBytes) == VfsResult::Ok);
    EOKAS_EXPECT(raw->put(coverDirKey("/"), dirBytes.data(), dirBytes.size()) == VfsResult::Ok);
    EOKAS_EXPECT(vfs.list("/", entries) == VfsResult::Ok);
    EOKAS_EXPECT(coverHas(entries, "shown.txt", VfsKind::File));
    EOKAS_EXPECT(!coverHas(entries, "hidden.txt", VfsKind::Tombstone));
    EOKAS_EXPECT(!coverHas(entries, "hidden.txt", VfsKind::File));
    EOKAS_EXPECT(vfs.readText("/hidden.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "hide");

    manifestTomb.path = "/shade";
    EOKAS_EXPECT(coverPlant(*raw, manifestTomb) == VfsResult::Ok);
    VfsDirEntry shade;
    shade.name = "shade";
    shade.kind = VfsKind::File;
    entries.clear();
    EOKAS_EXPECT(raw->get(coverDirKey("/"), dirBytes) == VfsResult::Ok);
    EOKAS_EXPECT(decodeDir(dirBytes.data(), dirBytes.size(), entries) == VfsResult::Ok);
    entries.push_back(shade);
    EOKAS_EXPECT(encodeDir(entries, dirBytes) == VfsResult::Ok);
    EOKAS_EXPECT(raw->put(coverDirKey("/"), dirBytes.data(), dirBytes.size()) == VfsResult::Ok);
    EOKAS_EXPECT(vfs.list("/", entries) == VfsResult::Ok);
    EOKAS_EXPECT(coverHas(entries, "shade", VfsKind::File));
    present = true;
    EOKAS_EXPECT(vfs.exists("/shade", present) == VfsResult::Ok);
    EOKAS_EXPECT(!present);
    EOKAS_EXPECT(vfs.open("/shade", VfsMode::Read, file) == VfsResult::NotFound);
    EOKAS_EXPECT(vfs.remove("/shade") == VfsResult::NotFound);
    EOKAS_EXPECT(vfs.createDir("/shade") == VfsResult::Ok);
    EOKAS_EXPECT(vfs.stat("/shade", stat) == VfsResult::Ok);
    EOKAS_EXPECT(stat.kind == VfsKind::Dir);
    EOKAS_EXPECT(vfs.list("/", entries) == VfsResult::Ok);
    EOKAS_EXPECT(coverHas(entries, "shade", VfsKind::Dir));

    u8_t junk[] = {9, 9, 9, 9};
    EOKAS_EXPECT(raw->put(coverManifestKey("/bad"), junk, 4) == VfsResult::Ok);
    present = true;
    EOKAS_EXPECT(vfs.exists("/bad", present) == VfsResult::Corrupt);
    EOKAS_EXPECT(present);
    stat.kind = VfsKind::Tombstone;
    stat.size = 42;
    EOKAS_EXPECT(vfs.stat("/bad", stat) == VfsResult::Corrupt);
    EOKAS_EXPECT(stat.kind == VfsKind::Tombstone && stat.size == 42);
    entries.push_back(shade);
    EOKAS_EXPECT(vfs.list("/bad", entries) == VfsResult::Corrupt);
    EOKAS_EXPECT(entries.empty());
    EOKAS_EXPECT(vfs.open("/bad", VfsMode::Read, file) == VfsResult::Corrupt);
    EOKAS_EXPECT(vfs.open("/bad", VfsMode::Write, file) == VfsResult::Corrupt);
    EOKAS_EXPECT(vfs.createDir("/bad") == VfsResult::Corrupt);
    EOKAS_EXPECT(vfs.remove("/bad") == VfsResult::Corrupt);
    EOKAS_EXPECT(vfs.readAll("/bad", zeroBytes) == VfsResult::Corrupt);
    EOKAS_EXPECT(vfs.writeAll("/bad", "a", 1) == VfsResult::Corrupt);
    EOKAS_EXPECT(vfs.readText("/bad", text) == VfsResult::Corrupt);
    EOKAS_EXPECT(vfs.writeText("/bad", "a") == VfsResult::Corrupt);
    EOKAS_EXPECT(vfs.verify("/bad", bad) == VfsResult::Corrupt);
    EOKAS_EXPECT(vfs.diff("/bad", "/wide.bin", spans) == VfsResult::Corrupt);
    EOKAS_EXPECT(vfs.attach("/bad/child", std::make_unique<VfsMemoryVolume>(), true) == VfsResult::Corrupt);

    VfsManifest wideManifest;
    std::vector<u8_t> manifestBytes;
    EOKAS_EXPECT(raw->get(coverManifestKey("/wide.bin"), manifestBytes) == VfsResult::Ok);
    EOKAS_EXPECT(decodeManifest(manifestBytes.data(), manifestBytes.size(), wideManifest) == VfsResult::Ok);
    EOKAS_EXPECT(wideManifest.chunks.size() == 3);
    std::vector<u8_t> chunk0;
    EOKAS_EXPECT(raw->get(coverChunkKey(wideManifest.chunks[0].hash), chunk0) == VfsResult::Ok);
    EOKAS_EXPECT(raw->remove(coverChunkKey(wideManifest.chunks[0].hash)) == VfsResult::Ok);
    EOKAS_EXPECT(vfs.verify("/wide.bin", bad) == VfsResult::Mismatch);
    EOKAS_EXPECT(bad.size() == 1 && bad[0] == 0);
    EOKAS_EXPECT(raw->put(coverChunkKey(wideManifest.chunks[0].hash), chunk0.data(), chunk0.size()) == VfsResult::Ok);
    wideManifest.size += 1;
    EOKAS_EXPECT(coverPlant(*raw, wideManifest) == VfsResult::Ok);
    EOKAS_EXPECT(vfs.verify("/wide.bin", bad) == VfsResult::Mismatch);
    EOKAS_EXPECT(bad.empty());
    wideManifest.size -= 1;
    wideManifest.hash = vfsHash("no", 2);
    EOKAS_EXPECT(coverPlant(*raw, wideManifest) == VfsResult::Ok);
    EOKAS_EXPECT(vfs.verify("/wide.bin", bad) == VfsResult::Mismatch);
    EOKAS_EXPECT(bad.empty());
    return 0;
}
