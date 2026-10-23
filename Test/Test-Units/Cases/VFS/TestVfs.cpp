#include "Unit.h"
#include "VFS/Header.h"

#include <cstdio>
#include <cstring>
#include <thread>

using namespace eokas;

static String testRoot(const char* tag)
{
    return String::format("vfs-unit-%s-%u", tag, Process::getPID());
}

static int countChunkFiles(const String& root)
{
    String dir = File::combinePath(root, "c");
    int count = 0;
    StringList folders = File::listFolderNames(dir);
    for (auto& folder : folders)
    {
        StringList files = File::listFileNames(File::combinePath(dir, folder));
        count += (int) files.size();
    }
    return count;
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

static bool refIs(VfsVolume& volume, const String& hash, u32_t expect)
{
    std::vector<u8_t> bytes;
    if (volume.get(String("k/") + hash, bytes) != VfsResult::Ok || bytes.size() != 4)
        return false;
    u32_t count = (u32_t) bytes[0] | ((u32_t) bytes[1] << 8) | ((u32_t) bytes[2] << 16) | ((u32_t) bytes[3] << 24);
    return count == expect;
}

static String chunkKey(const String& hash)
{
    return String("c/") + hash.substr(0, 2) + "/" + hash;
}

static VfsManifest sampleManifest()
{
    VfsManifest manifest;
    manifest.kind = VfsKind::File;
    manifest.path = "/cfg/a.txt";
    manifest.size = 3;
    manifest.hash = vfsHash("abc", 3);
    manifest.mtime = -42;
    manifest.chunkSize = VfsChunkBytes;
    VfsChunkRef chunk;
    chunk.hash = vfsHash("ab", 2);
    chunk.length = 2;
    manifest.chunks.push_back(chunk);
    chunk.hash = vfsHash("c", 1);
    chunk.length = 1;
    manifest.chunks.push_back(chunk);
    return manifest;
}

EOKAS_TEST_CASE(vfshash) {
    EOKAS_EXPECT(vfsHash(nullptr, 0) == sha256(String("")));
    EOKAS_EXPECT(vfsHash("hello", 5) == sha256(String("hello")));
    EOKAS_EXPECT(SHA256().compute((const u8_t*) "hello", 5) == sha256(String("hello")));
    EOKAS_EXPECT(vfsHash("hello", 5).length() == 64);
    u8_t raw[] = {'a', 0, 'b'};
    EOKAS_EXPECT(vfsHash(raw, 3) != vfsHash("a", 1));
    EOKAS_EXPECT(VfsPath::hash("/cfg/a.txt") == vfsHash("/cfg/a.txt", 10));
    return 0;
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

EOKAS_TEST_CASE(vfsman) {
    VfsManifest manifest = sampleManifest();
    std::vector<u8_t> bytes;
    EOKAS_EXPECT(encodeManifest(manifest, bytes) == VfsResult::Ok);
    VfsManifest back;
    EOKAS_EXPECT(decodeManifest(bytes.data(), bytes.size(), back) == VfsResult::Ok);
    EOKAS_EXPECT(back.kind == VfsKind::File);
    EOKAS_EXPECT(back.path == "/cfg/a.txt");
    EOKAS_EXPECT(back.size == 3);
    EOKAS_EXPECT(back.hash == manifest.hash);
    EOKAS_EXPECT(back.mtime == -42);
    EOKAS_EXPECT(back.chunkSize == VfsChunkBytes);
    EOKAS_EXPECT(back.chunks.size() == 2);
    EOKAS_EXPECT(back.chunks[1].length == 1);
    EOKAS_EXPECT(back.chunks[1].hash == manifest.chunks[1].hash);

    u8_t junk[] = {1, 2, 3, 4};
    EOKAS_EXPECT(decodeManifest(junk, 4, back) == VfsResult::Corrupt);
    EOKAS_EXPECT(decodeManifest(bytes.data(), 8, back) == VfsResult::Corrupt);
    std::vector<u8_t> version1 = bytes;
    version1[4] = 1;
    version1[5] = 0;
    EOKAS_EXPECT(decodeManifest(version1.data(), version1.size(), back) == VfsResult::Corrupt);
    manifest.hash = String("0123456789abcdef0123456789abcdef");
    EOKAS_EXPECT(encodeManifest(manifest, bytes) == VfsResult::Corrupt);
    manifest.hash = "short";
    EOKAS_EXPECT(encodeManifest(manifest, bytes) == VfsResult::Corrupt);

    std::vector<VfsDirEntry> entries;
    VfsDirEntry fileEntry;
    fileEntry.name = "a.txt";
    fileEntry.kind = VfsKind::File;
    VfsDirEntry dirEntry;
    dirEntry.name = "sub";
    dirEntry.kind = VfsKind::Dir;
    entries.push_back(fileEntry);
    entries.push_back(dirEntry);
    EOKAS_EXPECT(encodeDir(entries, bytes) == VfsResult::Ok);
    std::vector<VfsDirEntry> decoded;
    EOKAS_EXPECT(decodeDir(bytes.data(), bytes.size(), decoded) == VfsResult::Ok);
    EOKAS_EXPECT(decoded.size() == 2);
    EOKAS_EXPECT(decoded[0].name == "a.txt" && decoded[0].kind == VfsKind::File);
    EOKAS_EXPECT(decoded[1].kind == VfsKind::Dir);
    entries[0].name = "a/b";
    EOKAS_EXPECT(encodeDir(entries, bytes) == VfsResult::InvalidPath);
    EOKAS_EXPECT(decodeDir(junk, 4, decoded) == VfsResult::Corrupt);
    u8_t badKind[] = {1, 0, 0, 0, 1, 0, 'a', 9};
    EOKAS_EXPECT(decodeDir(badKind, 8, decoded) == VfsResult::Corrupt);

    VfsManifest left = sampleManifest();
    VfsManifest right = left;
    right.kind = VfsKind::Dir;
    VfsDiff kindDiff = diffManifest(left, right);
    EOKAS_EXPECT(!kindDiff.sameKind);
    EOKAS_EXPECT(kindDiff.spans.empty());

    right = left;
    right.chunks.clear();
    VfsDiff same = diffManifest(left, right);
    EOKAS_EXPECT(same.sameKind);
    EOKAS_EXPECT(same.sameHash);
    EOKAS_EXPECT(same.spans.size() == 1);
    EOKAS_EXPECT(same.spans[0].kind == VfsDiffKind::Equal);

    right.hash = vfsHash("x", 1);
    right.chunkSize = 10;
    VfsDiff sized = diffManifest(left, right);
    EOKAS_EXPECT(sized.spans.size() == 1);
    EOKAS_EXPECT(sized.spans[0].kind == VfsDiffKind::Modified);

    right = left;
    right.hash = vfsHash("x", 1);
    right.chunks.pop_back();
    right.size = 2;
    VfsDiff tail = diffManifest(left, right);
    EOKAS_EXPECT(tail.spans.size() == 2);
    EOKAS_EXPECT(tail.spans[0].kind == VfsDiffKind::Equal);
    EOKAS_EXPECT(tail.spans[1].kind == VfsDiffKind::LeftOnly);
    VfsDiff flipped = diffManifest(right, left);
    EOKAS_EXPECT(flipped.spans.back().kind == VfsDiffKind::RightOnly);

    auto volume = std::make_shared<VfsMemoryVolume>();
    VfsChunkStore store(volume);
    VfsManifest empty;
    empty.kind = VfsKind::File;
    empty.path = "/empty";
    empty.size = 0;
    empty.hash = vfsHash(nullptr, 0);
    empty.chunkSize = VfsChunkBytes;
    EOKAS_EXPECT(store.writeManifest(empty) == VfsResult::Ok);
    VfsManifest loaded;
    EOKAS_EXPECT(store.readManifest("/empty", loaded) == VfsResult::Ok);
    EOKAS_EXPECT(store.readManifest("/missing", loaded) == VfsResult::NotFound);
    std::vector<u8_t> stored;
    EOKAS_EXPECT(volume->get(String("m/") + VfsPath::hash("/empty"), stored) == VfsResult::Ok);
    EOKAS_EXPECT(volume->put(String("m/") + VfsPath::hash("/other"), stored.data(), stored.size()) == VfsResult::Ok);
    EOKAS_EXPECT(store.readManifest("/other", loaded) == VfsResult::Corrupt);

    std::vector<u32_t> bad;
    empty.hash = vfsHash("x", 1);
    EOKAS_EXPECT(store.verify(empty, bad) == VfsResult::Mismatch);
    EOKAS_EXPECT(bad.empty());
    empty.hash = vfsHash(nullptr, 0);
    EOKAS_EXPECT(store.verify(empty, bad) == VfsResult::Ok);

    String chunk = vfsHash("ab", 2);
    EOKAS_EXPECT(store.putChunk("short", "ab", 2) == VfsResult::Corrupt);
    EOKAS_EXPECT(store.putChunk(chunk, "ab", 2) == VfsResult::Ok);
    EOKAS_EXPECT(store.putChunk(chunk, "zz", 2) == VfsResult::Ok);
    std::vector<u8_t> body;
    EOKAS_EXPECT(store.getChunk(chunk, body) == VfsResult::Ok);
    EOKAS_EXPECT(body.size() == 2 && body[0] == 'a');
    EOKAS_EXPECT(store.addRefs(chunk, 0) == VfsResult::Ok);
    EOKAS_EXPECT(store.addRefs(chunk, 1) == VfsResult::Ok);
    EOKAS_EXPECT(store.releaseRefs(chunk, 0) == VfsResult::Ok);
    EOKAS_EXPECT(store.releaseRefs(chunk, 2) == VfsResult::Corrupt);
    EOKAS_EXPECT(store.releaseRefs(vfsHash("no", 2), 1) == VfsResult::Corrupt);
    EOKAS_EXPECT(store.releaseRefs(chunk, 1) == VfsResult::Ok);
    bool present = true;
    EOKAS_EXPECT(volume->exists(chunkKey(chunk), present) == VfsResult::Ok);
    EOKAS_EXPECT(!present);
    EOKAS_EXPECT(store.upsertChild("/", "", VfsKind::File) == VfsResult::InvalidPath);
    EOKAS_EXPECT(store.upsertChild("/", "a/b", VfsKind::File) == VfsResult::InvalidPath);
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
    Vfs vfs;
    EOKAS_EXPECT(vfs.attach("/", std::make_unique<VfsMemoryVolume>(), true) == VfsResult::Ok);
    EOKAS_EXPECT(vfs.writeText("/t.txt", "hello") == VfsResult::Ok);
    std::vector<u8_t> bytes;
    EOKAS_EXPECT(vfs.readAll("/t.txt", bytes) == VfsResult::Ok);
    EOKAS_EXPECT(bytes.size() == 5);
    EOKAS_EXPECT(std::memcmp(bytes.data(), "hello", 5) == 0);

    u8_t raw[] = {'a', 0, 'b'};
    EOKAS_EXPECT(vfs.writeAll("/b.bin", raw, 3) == VfsResult::Ok);
    std::vector<u8_t> back;
    EOKAS_EXPECT(vfs.readAll("/b.bin", back) == VfsResult::Ok);
    EOKAS_EXPECT(back.size() == 3);
    EOKAS_EXPECT(back[0] == 'a' && back[1] == 0 && back[2] == 'b');

    String text;
    EOKAS_EXPECT(vfs.readText("/t.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "hello");

    VfsStat stat;
    EOKAS_EXPECT(vfs.stat("/t.txt", stat) == VfsResult::Ok);
    EOKAS_EXPECT(stat.kind == VfsKind::File);
    EOKAS_EXPECT(stat.size == 5);
    EOKAS_EXPECT(stat.hash == vfsHash("hello", 5));
    EOKAS_EXPECT(stat.mtime != 0);
    EOKAS_EXPECT(vfs.stat("/", stat) == VfsResult::Ok);
    EOKAS_EXPECT(stat.kind == VfsKind::Dir);
    EOKAS_EXPECT(stat.size == 0);
    EOKAS_EXPECT(stat.hash == vfsHash(nullptr, 0));
    EOKAS_EXPECT(stat.mtime == 0);
    return 0;
}

EOKAS_TEST_CASE(vfsdisk) {
    String root = testRoot("disk");
    {
        Vfs vfs;
        EOKAS_EXPECT(vfs.attach("/", std::make_unique<VfsDiskVolume>(root), true) == VfsResult::Ok);
        EOKAS_EXPECT(vfs.writeText("/note.txt", "disk") == VfsResult::Ok);
    }
    Vfs again;
    EOKAS_EXPECT(again.attach("/", std::make_unique<VfsDiskVolume>(root), false) == VfsResult::Ok);
    String text;
    EOKAS_EXPECT(again.readText("/note.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "disk");
    EOKAS_EXPECT(again.writeText("/note.txt", "no") == VfsResult::Access);
    return 0;
}

EOKAS_TEST_CASE(vfschunk) {
    String root = testRoot("chunks");
    Vfs vfs;
    EOKAS_EXPECT(vfs.attach("/", std::make_unique<VfsDiskVolume>(root), true) == VfsResult::Ok);

    std::vector<u8_t> left(VfsChunkBytes + 4, 1);
    std::vector<u8_t> right = left;
    right.back() = 2;
    EOKAS_EXPECT(vfs.writeAll("/left.bin", left.data(), left.size()) == VfsResult::Ok);
    EOKAS_EXPECT(vfs.writeAll("/right.bin", right.data(), right.size()) == VfsResult::Ok);
    EOKAS_EXPECT(countChunkFiles(root) == 3);

    VfsDiff same;
    EOKAS_EXPECT(vfs.diff("/left.bin", "/left.bin", same) == VfsResult::Ok);
    EOKAS_EXPECT(same.sameKind);
    EOKAS_EXPECT(same.sameHash);
    EOKAS_EXPECT(same.spans.size() == 1);
    EOKAS_EXPECT(same.spans[0].kind == VfsDiffKind::Equal);

    VfsDiff changed;
    EOKAS_EXPECT(vfs.diff("/left.bin", "/right.bin", changed) == VfsResult::Ok);
    EOKAS_EXPECT(changed.sameKind);
    EOKAS_EXPECT(!changed.sameHash);
    bool modified = false;
    bool equal = false;
    for (auto& span : changed.spans)
    {
        if (span.kind == VfsDiffKind::Modified)
            modified = true;
        if (span.kind == VfsDiffKind::Equal)
            equal = true;
    }
    EOKAS_EXPECT(modified);
    EOKAS_EXPECT(equal);

    std::vector<u32_t> bad;
    EOKAS_EXPECT(vfs.verify("/left.bin", bad) == VfsResult::Ok);
    EOKAS_EXPECT(vfs.verify("/right.bin", bad) == VfsResult::Ok);

    String tailHash = vfsHash(left.data() + VfsChunkBytes, 4);
    String tailPath = File::combinePath(File::combinePath(File::combinePath(root, "c"), tailHash.substr(0, 2)), tailHash);
    FILE* handle = std::fopen(tailPath.cstr(), "r+b");
    EOKAS_EXPECT(handle != nullptr);
    int value = std::fgetc(handle);
    EOKAS_EXPECT(value != EOF);
    std::fseek(handle, 0, SEEK_SET);
    std::fputc(value ^ 0x5A, handle);
    std::fclose(handle);

    bad.clear();
    EOKAS_EXPECT(vfs.verify("/left.bin", bad) == VfsResult::Mismatch);
    EOKAS_EXPECT(bad.size() == 1);
    EOKAS_EXPECT(bad[0] == 1);
    EOKAS_EXPECT(vfs.verify("/right.bin", bad) == VfsResult::Ok);
    return 0;
}

EOKAS_TEST_CASE(vfsref) {
    auto memory = std::make_unique<VfsMemoryVolume>();
    VfsMemoryVolume* raw = memory.get();
    Vfs vfs;
    EOKAS_EXPECT(vfs.attach("/", std::move(memory), true) == VfsResult::Ok);
    const char* body = "same-bytes";
    EOKAS_EXPECT(vfs.writeText("/a.txt", body) == VfsResult::Ok);
    EOKAS_EXPECT(vfs.writeText("/b.txt", body) == VfsResult::Ok);
    String hash = vfsHash(body, std::strlen(body));
    EOKAS_EXPECT(keyCount(*raw, "c/") == 1);
    EOKAS_EXPECT(refIs(*raw, hash, 2));
    EOKAS_EXPECT(vfs.writeText("/a.txt", body) == VfsResult::Ok);
    EOKAS_EXPECT(keyCount(*raw, "c/") == 1);
    EOKAS_EXPECT(refIs(*raw, hash, 2));

    EOKAS_EXPECT(vfs.writeText("/a.txt", "other") == VfsResult::Ok);
    String next = vfsHash("other", 5);
    EOKAS_EXPECT(refIs(*raw, hash, 1));
    EOKAS_EXPECT(refIs(*raw, next, 1));
    EOKAS_EXPECT(keyCount(*raw, "c/") == 2);
    bool present = true;
    EOKAS_EXPECT(raw->exists(chunkKey(hash), present) == VfsResult::Ok);
    EOKAS_EXPECT(present);

    EOKAS_EXPECT(vfs.remove("/b.txt") == VfsResult::Ok);
    EOKAS_EXPECT(raw->exists(chunkKey(hash), present) == VfsResult::Ok);
    EOKAS_EXPECT(!present);
    EOKAS_EXPECT(refIs(*raw, next, 1));
    EOKAS_EXPECT(vfs.remove("/a.txt") == VfsResult::Ok);
    EOKAS_EXPECT(keyCount(*raw, "c/") == 0);
    EOKAS_EXPECT(keyCount(*raw, "k/") == 0);

    EOKAS_EXPECT(vfs.writeAll("/empty", nullptr, 0) == VfsResult::Ok);
    EOKAS_EXPECT(vfs.writeAll("/empty2", nullptr, 0) == VfsResult::Ok);
    EOKAS_EXPECT(keyCount(*raw, "c/") == 0);
    VfsStat stat;
    EOKAS_EXPECT(vfs.stat("/empty", stat) == VfsResult::Ok);
    EOKAS_EXPECT(stat.kind == VfsKind::File);
    EOKAS_EXPECT(stat.size == 0);
    EOKAS_EXPECT(stat.hash == vfsHash(nullptr, 0));
    std::vector<u32_t> bad;
    EOKAS_EXPECT(vfs.verify("/empty", bad) == VfsResult::Ok);
    return 0;
}

EOKAS_TEST_CASE(vfsio) {
    Vfs vfs;
    EOKAS_EXPECT(vfs.attach("/", std::make_unique<VfsMemoryVolume>(), true) == VfsResult::Ok);
    std::vector<u8_t> body(VfsChunkBytes + 4, 7);
    body[VfsChunkBytes] = 9;
    body[VfsChunkBytes + 1] = 9;
    body[VfsChunkBytes + 2] = 9;
    body[VfsChunkBytes + 3] = 9;
    EOKAS_EXPECT(vfs.writeAll("/span.bin", body.data(), body.size()) == VfsResult::Ok);

    VfsFile closed;
    u64_t count = 1;
    EOKAS_EXPECT(closed.read(nullptr, 0, count) == VfsResult::Closed);
    EOKAS_EXPECT(!closed.isOpen());
    EOKAS_EXPECT(closed.tell() == 0);
    EOKAS_EXPECT(closed.size() == 0);
    EOKAS_EXPECT(closed.close() == VfsResult::Ok);

    VfsFile file;
    EOKAS_EXPECT(vfs.open("/missing", VfsMode::Read, file) == VfsResult::NotFound);
    EOKAS_EXPECT(vfs.open("/", VfsMode::Read, file) == VfsResult::Access);
    EOKAS_EXPECT(vfs.open("/span.bin", VfsMode::Read, file) == VfsResult::Ok);
    EOKAS_EXPECT(file.isOpen());
    EOKAS_EXPECT(file.size() == body.size());
    EOKAS_EXPECT(file.write("x", 1, count) == VfsResult::Access);
    EOKAS_EXPECT(file.read(nullptr, 0, count) == VfsResult::Ok);
    EOKAS_EXPECT(count == 0);
    EOKAS_EXPECT(file.seek((i64_t) VfsChunkBytes - 2, VfsSeek::Begin) == VfsResult::Ok);
    u8_t span[16];
    EOKAS_EXPECT(file.read(span, 6, count) == VfsResult::Ok);
    EOKAS_EXPECT(count == 6);
    EOKAS_EXPECT(span[0] == 7 && span[1] == 7 && span[2] == 9 && span[5] == 9);
    EOKAS_EXPECT(file.tell() == VfsChunkBytes + 4);
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
    EOKAS_EXPECT(vfs.open("/pad.bin", VfsMode::Write, writer) == VfsResult::Ok);
    EOKAS_EXPECT(writer.seek(2, VfsSeek::Begin) == VfsResult::Ok);
    EOKAS_EXPECT(writer.write("Z", 1, count) == VfsResult::Ok);
    EOKAS_EXPECT(writer.flush() == VfsResult::Ok);
    EOKAS_EXPECT(writer.flush() == VfsResult::Ok);
    EOKAS_EXPECT(writer.close() == VfsResult::Ok);
    std::vector<u8_t> padded;
    EOKAS_EXPECT(vfs.readAll("/pad.bin", padded) == VfsResult::Ok);
    EOKAS_EXPECT(padded.size() == 3);
    EOKAS_EXPECT(padded[0] == 0 && padded[1] == 0 && padded[2] == 'Z');

    EOKAS_EXPECT(vfs.writeText("/edit.txt", "abcd") == VfsResult::Ok);
    VfsStat before;
    EOKAS_EXPECT(vfs.stat("/edit.txt", before) == VfsResult::Ok);
    VfsFile edit;
    EOKAS_EXPECT(vfs.open("/edit.txt", VfsMode::ReadWrite, edit) == VfsResult::Ok);
    EOKAS_EXPECT(edit.close() == VfsResult::Ok);
    VfsStat after;
    EOKAS_EXPECT(vfs.stat("/edit.txt", after) == VfsResult::Ok);
    EOKAS_EXPECT(after.hash == before.hash);
    EOKAS_EXPECT(after.mtime == before.mtime);
    EOKAS_EXPECT(vfs.open("/edit.txt", VfsMode::ReadWrite, edit) == VfsResult::Ok);
    EOKAS_EXPECT(edit.seek(1, VfsSeek::Begin) == VfsResult::Ok);
    EOKAS_EXPECT(edit.write("XY", 2, count) == VfsResult::Ok);
    EOKAS_EXPECT(edit.close() == VfsResult::Ok);
    EOKAS_EXPECT(vfs.readText("/edit.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "aXYd");

    EOKAS_EXPECT(vfs.open("/edit.txt", VfsMode::Append, edit) == VfsResult::Ok);
    EOKAS_EXPECT(edit.tell() == 4);
    EOKAS_EXPECT(edit.write("!", 1, count) == VfsResult::Ok);
    EOKAS_EXPECT(edit.close() == VfsResult::Ok);
    EOKAS_EXPECT(vfs.readText("/edit.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "aXYd!");

    EOKAS_EXPECT(vfs.open("/edit.txt", VfsMode::ReadWrite, edit) == VfsResult::Ok);
    EOKAS_EXPECT(edit.writeText("zz") == VfsResult::Ok);
    EOKAS_EXPECT(edit.close() == VfsResult::Ok);
    EOKAS_EXPECT(vfs.readText("/edit.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "zzYd!");

    EOKAS_EXPECT(vfs.open("/edit.txt", VfsMode::Append, edit) == VfsResult::Ok);
    EOKAS_EXPECT(edit.writeText("?") == VfsResult::Ok);
    EOKAS_EXPECT(edit.close() == VfsResult::Ok);
    EOKAS_EXPECT(vfs.readText("/edit.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "zzYd!?");

    u8_t rawNul[] = {'a', 0, 'b'};
    EOKAS_EXPECT(vfs.writeAll("/nul.bin", rawNul, 3) == VfsResult::Ok);
    EOKAS_EXPECT(vfs.readText("/nul.bin", text) == VfsResult::Corrupt);
    std::vector<u8_t> nulBytes;
    EOKAS_EXPECT(vfs.readAll("/nul.bin", nulBytes) == VfsResult::Ok);
    EOKAS_EXPECT(nulBytes.size() == 3 && nulBytes[0] == 'a' && nulBytes[1] == 0 && nulBytes[2] == 'b');

    VfsFile fresh;
    EOKAS_EXPECT(vfs.open("/new.bin", VfsMode::ReadWrite, fresh) == VfsResult::Ok);
    EOKAS_EXPECT(fresh.close() == VfsResult::Ok);
    bool exists = false;
    EOKAS_EXPECT(vfs.exists("/new.bin", exists) == VfsResult::Ok);
    EOKAS_EXPECT(exists);
    EOKAS_EXPECT(vfs.open("/new.bin", VfsMode::Read, fresh) == VfsResult::Ok);
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
    Vfs vfs;
    EOKAS_EXPECT(vfs.attach("/", std::move(gate), true) == VfsResult::Ok);

    VfsFile file;
    EOKAS_EXPECT(file.close() == VfsResult::Ok);
    EOKAS_EXPECT(vfs.open("/hold.txt", VfsMode::Write, file) == VfsResult::Ok);
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
    EOKAS_EXPECT(vfs.readText("/hold.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "hi");

    EOKAS_EXPECT(vfs.open("/hold.txt", VfsMode::ReadWrite, file) == VfsResult::Ok);
    EOKAS_EXPECT(file.write("Z", 1, count) == VfsResult::Ok);
    raw->rejectPut = true;
    EOKAS_EXPECT(vfs.open("/other.txt", VfsMode::Write, file) == VfsResult::Io);
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
    EOKAS_EXPECT(vfs.readText("/hold.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "Zi");
    bool exists = true;
    EOKAS_EXPECT(vfs.exists("/other.txt", exists) == VfsResult::Ok);
    EOKAS_EXPECT(!exists);
    return 0;
}

EOKAS_TEST_CASE(vfsretry) {
    auto gate = std::make_unique<PutGateVolume>();
    PutGateVolume* raw = gate.get();
    Vfs vfs;
    EOKAS_EXPECT(vfs.attach("/", std::move(gate), true) == VfsResult::Ok);
    EOKAS_EXPECT(vfs.writeText("/a.txt", "hi") == VfsResult::Ok);
    EOKAS_EXPECT(vfs.writeText("/b.txt", "hi") == VfsResult::Ok);
    String oldHash = vfsHash("hi", 2);
    String nextHash = vfsHash("yo", 2);
    EOKAS_EXPECT(refIs(*raw, oldHash, 2));

    VfsFile file;
    u64_t count = 0;
    EOKAS_EXPECT(vfs.open("/a.txt", VfsMode::ReadWrite, file) == VfsResult::Ok);
    EOKAS_EXPECT(file.write("yo", 2, count) == VfsResult::Ok);
    raw->failManifestPuts = 1;
    EOKAS_EXPECT(file.flush() == VfsResult::Io);
    EOKAS_EXPECT(refIs(*raw, nextHash, 1));
    EOKAS_EXPECT(refIs(*raw, oldHash, 2));
    String text;
    EOKAS_EXPECT(vfs.readText("/a.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "hi");
    EOKAS_EXPECT(file.flush() == VfsResult::Ok);
    EOKAS_EXPECT(refIs(*raw, nextHash, 1));
    EOKAS_EXPECT(refIs(*raw, oldHash, 1));
    EOKAS_EXPECT(file.close() == VfsResult::Ok);
    EOKAS_EXPECT(vfs.readText("/a.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "yo");
    EOKAS_EXPECT(vfs.readText("/b.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "hi");

    EOKAS_EXPECT(vfs.open("/a.txt", VfsMode::ReadWrite, file) == VfsResult::Ok);
    EOKAS_EXPECT(file.write("hi", 2, count) == VfsResult::Ok);
    raw->failRemovePrefix = String("k/") + nextHash;
    raw->failRemoveTimes = 1;
    EOKAS_EXPECT(file.flush() == VfsResult::Io);
    EOKAS_EXPECT(refIs(*raw, nextHash, 1));
    EOKAS_EXPECT(refIs(*raw, oldHash, 2));
    EOKAS_EXPECT(file.flush() == VfsResult::Ok);
    bool present = true;
    EOKAS_EXPECT(raw->exists(String("k/") + nextHash, present) == VfsResult::Ok);
    EOKAS_EXPECT(!present);
    EOKAS_EXPECT(refIs(*raw, oldHash, 2));
    EOKAS_EXPECT(file.close() == VfsResult::Ok);
    EOKAS_EXPECT(vfs.readText("/a.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "hi");
    EOKAS_EXPECT(vfs.readText("/b.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "hi");

    raw->failRemovePrefix = String("m/");
    raw->failRemoveTimes = 1;
    EOKAS_EXPECT(vfs.remove("/b.txt") == VfsResult::Io);
    EOKAS_EXPECT(vfs.readText("/b.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "hi");
    EOKAS_EXPECT(refIs(*raw, oldHash, 2));
    EOKAS_EXPECT(vfs.remove("/b.txt") == VfsResult::Ok);
    EOKAS_EXPECT(refIs(*raw, oldHash, 1));
    EOKAS_EXPECT(vfs.readText("/a.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "hi");
    return 0;
}

EOKAS_TEST_CASE(vfsdir) {
    Vfs vfs;
    EOKAS_EXPECT(vfs.attach("/", std::make_unique<VfsMemoryVolume>(), true) == VfsResult::Ok);
    bool exists = false;
    EOKAS_EXPECT(vfs.exists("/", exists) == VfsResult::Ok);
    EOKAS_EXPECT(exists);
    EOKAS_EXPECT(vfs.exists("/missing", exists) == VfsResult::Ok);
    EOKAS_EXPECT(!exists);
    EOKAS_EXPECT(vfs.exists("relative", exists) == VfsResult::InvalidPath);
    EOKAS_EXPECT(vfs.exists("/a/../b", exists) == VfsResult::InvalidPath);
    VfsStat stat;
    EOKAS_EXPECT(vfs.stat("/missing", stat) == VfsResult::NotFound);
    std::vector<VfsDirEntry> entries;
    EOKAS_EXPECT(vfs.list("/", entries) == VfsResult::Ok);
    EOKAS_EXPECT(entries.empty());
    EOKAS_EXPECT(vfs.list("/missing", entries) == VfsResult::NotFound);
    EOKAS_EXPECT(vfs.createDir("/") == VfsResult::Ok);
    EOKAS_EXPECT(vfs.remove("/") == VfsResult::Access);
    EOKAS_EXPECT(vfs.createDir("/box/inner") == VfsResult::Ok);
    EOKAS_EXPECT(vfs.createDir("/box/inner") == VfsResult::Ok);
    EOKAS_EXPECT(vfs.list("/", entries) == VfsResult::Ok);
    EOKAS_EXPECT(containsKind(entries, "box", VfsKind::Dir));
    EOKAS_EXPECT(vfs.list("/box", entries) == VfsResult::Ok);
    EOKAS_EXPECT(containsKind(entries, "inner", VfsKind::Dir));
    EOKAS_EXPECT(vfs.writeText("/box/inner/a.txt", "file") == VfsResult::Ok);
    EOKAS_EXPECT(vfs.list("/box/inner", entries) == VfsResult::Ok);
    EOKAS_EXPECT(containsKind(entries, "a.txt", VfsKind::File));
    EOKAS_EXPECT(vfs.list("/box/inner/a.txt", entries) == VfsResult::Access);
    VfsFile file;
    EOKAS_EXPECT(vfs.open("/box", VfsMode::Read, file) == VfsResult::Access);
    EOKAS_EXPECT(vfs.remove("/box") == VfsResult::NotEmpty);
    EOKAS_EXPECT(vfs.exists("/box", exists) == VfsResult::Ok);
    EOKAS_EXPECT(exists);
    EOKAS_EXPECT(vfs.remove("/box/inner/a.txt") == VfsResult::Ok);
    EOKAS_EXPECT(vfs.list("/box/inner", entries) == VfsResult::Ok);
    EOKAS_EXPECT(entries.empty());
    EOKAS_EXPECT(vfs.remove("/box/inner") == VfsResult::Ok);
    EOKAS_EXPECT(vfs.remove("/box") == VfsResult::Ok);
    EOKAS_EXPECT(vfs.exists("/box", exists) == VfsResult::Ok);
    EOKAS_EXPECT(!exists);

    EOKAS_EXPECT(vfs.writeText("/a.txt", "file") == VfsResult::Ok);
    EOKAS_EXPECT(vfs.createDir("/a.txt") == VfsResult::AlreadyExists);
    EOKAS_EXPECT(vfs.createDir("/a.txt/child") == VfsResult::AlreadyExists);
    EOKAS_EXPECT(vfs.writeAll("/empty", nullptr, 0) == VfsResult::Ok);
    EOKAS_EXPECT(vfs.createDir("/empty-dir") == VfsResult::Ok);
    VfsStat fileStat;
    VfsStat dirStat;
    EOKAS_EXPECT(vfs.stat("/empty", fileStat) == VfsResult::Ok);
    EOKAS_EXPECT(vfs.stat("/empty-dir", dirStat) == VfsResult::Ok);
    EOKAS_EXPECT(fileStat.kind == VfsKind::File);
    EOKAS_EXPECT(dirStat.kind == VfsKind::Dir);
    EOKAS_EXPECT(fileStat.size == 0 && dirStat.size == 0);
    EOKAS_EXPECT(fileStat.hash == dirStat.hash);
    VfsDiff diff;
    EOKAS_EXPECT(vfs.diff("/empty", "/empty-dir", diff) == VfsResult::Ok);
    EOKAS_EXPECT(!diff.sameKind);
    EOKAS_EXPECT(diff.sameHash);
    EOKAS_EXPECT(diff.spans.empty());
    EOKAS_EXPECT(vfs.diff("/missing", "/empty", diff) == VfsResult::NotFound);
    EOKAS_EXPECT(vfs.diff("bad", "/empty", diff) == VfsResult::InvalidPath);
    std::vector<u32_t> bad;
    EOKAS_EXPECT(vfs.verify("/empty-dir", bad) == VfsResult::Ok);
    EOKAS_EXPECT(vfs.verify("/missing", bad) == VfsResult::NotFound);
    return 0;
}

EOKAS_TEST_CASE(vfsover) {
    String baseRoot = testRoot("base");
    {
        Vfs prep;
        EOKAS_EXPECT(prep.attach("/", std::make_unique<VfsDiskVolume>(baseRoot), true) == VfsResult::Ok);
        EOKAS_EXPECT(prep.writeText("/keep.txt", "keep") == VfsResult::Ok);
        EOKAS_EXPECT(prep.writeText("/save/old.txt", "old") == VfsResult::Ok);
    }

    auto save = std::make_unique<VfsMemoryVolume>();
    VfsMemoryVolume* saveRaw = save.get();
    Vfs vfs;
    EOKAS_EXPECT(vfs.attach("/", std::make_unique<VfsDiskVolume>(baseRoot), true) == VfsResult::Ok);
    EOKAS_EXPECT(vfs.attach("/save", std::move(save), true) == VfsResult::Ok);

    String text;
    EOKAS_EXPECT(vfs.readText("/keep.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "keep");
    bool present = true;
    EOKAS_EXPECT(vfs.exists("/save/old.txt", present) == VfsResult::Ok);
    EOKAS_EXPECT(!present);
    EOKAS_EXPECT(vfs.writeText("/save/slot.dat", "mem") == VfsResult::Ok);
    EOKAS_EXPECT(vfs.writeText("/keep.txt", "root") == VfsResult::Ok);
    EOKAS_EXPECT(vfs.readText("/save/slot.dat", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "mem");
    EOKAS_EXPECT(vfs.readText("/keep.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "root");
    EOKAS_EXPECT(keyCount(*saveRaw, "m/") > 0);

    VfsStat stat;
    EOKAS_EXPECT(vfs.stat("/save", stat) == VfsResult::Ok);
    EOKAS_EXPECT(stat.kind == VfsKind::Dir);
    EOKAS_EXPECT(stat.size == 0);
    EOKAS_EXPECT(stat.hash == vfsHash(nullptr, 0));
    EOKAS_EXPECT(stat.mtime == 0);
    EOKAS_EXPECT(vfs.createDir("/save") == VfsResult::Ok);
    EOKAS_EXPECT(vfs.remove("/save") == VfsResult::Access);
    VfsFile file;
    EOKAS_EXPECT(vfs.open("/save", VfsMode::Read, file) == VfsResult::Access);

    std::vector<VfsDirEntry> entries;
    EOKAS_EXPECT(vfs.list("/", entries) == VfsResult::Ok);
    EOKAS_EXPECT(containsKind(entries, "save", VfsKind::Dir));
    EOKAS_EXPECT(containsKind(entries, "keep.txt", VfsKind::File));
    EOKAS_EXPECT(vfs.list("/save", entries) == VfsResult::Ok);
    EOKAS_EXPECT(containsKind(entries, "slot.dat", VfsKind::File));
    EOKAS_EXPECT(!containsName(entries, "old.txt"));

    EOKAS_EXPECT(vfs.detach("/save") == VfsResult::Ok);
    EOKAS_EXPECT(vfs.readText("/save/old.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "old");
    EOKAS_EXPECT(vfs.readText("/keep.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "root");
    return 0;
}

EOKAS_TEST_CASE(vfsatt) {
    Vfs bare;
    bool exists = false;
    std::vector<VfsDirEntry> entries;
    EOKAS_EXPECT(bare.exists("/", exists) == VfsResult::Ok);
    EOKAS_EXPECT(exists);
    EOKAS_EXPECT(bare.list("/", entries) == VfsResult::Ok);
    EOKAS_EXPECT(entries.empty());
    EOKAS_EXPECT(bare.writeText("/a.txt", "no") == VfsResult::NotFound);
    EOKAS_EXPECT(bare.attach("", std::make_unique<VfsMemoryVolume>(), true) == VfsResult::InvalidPath);
    EOKAS_EXPECT(bare.attach("mem", std::make_unique<VfsMemoryVolume>(), true) == VfsResult::InvalidPath);
    EOKAS_EXPECT(bare.attach("/", std::unique_ptr<VfsVolume>(), true) == VfsResult::Io);
    EOKAS_EXPECT(bare.attach("/", std::make_unique<VfsMemoryVolume>(), false) == VfsResult::Ok);
    EOKAS_EXPECT(bare.attach("/", std::make_unique<VfsMemoryVolume>(), true) == VfsResult::AlreadyExists);
    EOKAS_EXPECT(bare.writeText("/a.txt", "no") == VfsResult::Access);
    EOKAS_EXPECT(bare.createDir("/d") == VfsResult::Access);
    EOKAS_EXPECT(bare.detach("/missing") == VfsResult::NotFound);
    EOKAS_EXPECT(bare.detach("/") == VfsResult::Ok);

    auto root = std::make_unique<VfsMemoryVolume>();
    auto save = std::make_unique<VfsMemoryVolume>();
    auto assets = std::make_unique<VfsMemoryVolume>();
    auto ui = std::make_unique<VfsMemoryVolume>();
    VfsMemoryVolume* rootRaw = root.get();
    VfsMemoryVolume* saveRaw = save.get();
    VfsMemoryVolume* assetsRaw = assets.get();
    VfsMemoryVolume* uiRaw = ui.get();
    Vfs vfs;
    EOKAS_EXPECT(vfs.attach("/", std::move(root), true) == VfsResult::Ok);
    EOKAS_EXPECT(vfs.attach("/save", std::move(save), true) == VfsResult::Ok);
    EOKAS_EXPECT(vfs.attach("/assets", std::move(assets), true) == VfsResult::Ok);
    EOKAS_EXPECT(vfs.attach("/assets/ui", std::move(ui), true) == VfsResult::Ok);
    EOKAS_EXPECT(vfs.attach("/assets/ui", std::make_unique<VfsMemoryVolume>(), true) == VfsResult::AlreadyExists);

    EOKAS_EXPECT(vfs.writeText("/save2/a.txt", "root-file") == VfsResult::Ok);
    EOKAS_EXPECT(vfs.writeText("/save/a.txt", "save-file") == VfsResult::Ok);
    EOKAS_EXPECT(vfs.writeText("/assets/b.txt", "asset") == VfsResult::Ok);
    EOKAS_EXPECT(vfs.writeText("/assets/ui/a.txt", "ui") == VfsResult::Ok);
    String text;
    EOKAS_EXPECT(vfs.readText("/save2/a.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "root-file");
    EOKAS_EXPECT(vfs.readText("/save/a.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "save-file");
    EOKAS_EXPECT(vfs.readText("/assets/b.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "asset");
    EOKAS_EXPECT(vfs.readText("/assets/ui/a.txt", text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "ui");

    bool present = false;
    EOKAS_EXPECT(rootRaw->exists(String("m/") + VfsPath::hash("/save2/a.txt"), present) == VfsResult::Ok);
    EOKAS_EXPECT(present);
    EOKAS_EXPECT(saveRaw->exists(String("m/") + VfsPath::hash("/save2/a.txt"), present) == VfsResult::Ok);
    EOKAS_EXPECT(!present);
    EOKAS_EXPECT(saveRaw->exists(String("m/") + VfsPath::hash("/save/a.txt"), present) == VfsResult::Ok);
    EOKAS_EXPECT(present);
    EOKAS_EXPECT(assetsRaw->exists(String("m/") + VfsPath::hash("/assets/b.txt"), present) == VfsResult::Ok);
    EOKAS_EXPECT(present);
    EOKAS_EXPECT(assetsRaw->exists(String("m/") + VfsPath::hash("/assets/ui/a.txt"), present) == VfsResult::Ok);
    EOKAS_EXPECT(!present);
    EOKAS_EXPECT(uiRaw->exists(String("m/") + VfsPath::hash("/assets/ui/a.txt"), present) == VfsResult::Ok);
    EOKAS_EXPECT(present);

    EOKAS_EXPECT(vfs.list("/assets", entries) == VfsResult::Ok);
    EOKAS_EXPECT(containsKind(entries, "ui", VfsKind::Dir));
    EOKAS_EXPECT(containsKind(entries, "b.txt", VfsKind::File));
    EOKAS_EXPECT(vfs.list("/assets/ui", entries) == VfsResult::Ok);
    EOKAS_EXPECT(containsKind(entries, "a.txt", VfsKind::File));

    EOKAS_EXPECT(vfs.remove("/assets/b.txt") == VfsResult::Ok);
    EOKAS_EXPECT(vfs.remove("/assets") == VfsResult::Access);
    EOKAS_EXPECT(vfs.remove("/assets/ui") == VfsResult::Access);
    EOKAS_EXPECT(vfs.createDir("/box") == VfsResult::Ok);
    EOKAS_EXPECT(vfs.attach("/box/ui", std::make_unique<VfsMemoryVolume>(), true) == VfsResult::Ok);
    EOKAS_EXPECT(vfs.remove("/box") == VfsResult::NotEmpty);
    EOKAS_EXPECT(vfs.detach("/box/ui") == VfsResult::Ok);
    EOKAS_EXPECT(vfs.remove("/box") == VfsResult::Ok);
    EOKAS_EXPECT(vfs.detach("/assets/ui") == VfsResult::Ok);

    EOKAS_EXPECT(vfs.writeText("/blocked", "file") == VfsResult::Ok);
    EOKAS_EXPECT(vfs.attach("/blocked", std::make_unique<VfsMemoryVolume>(), true) == VfsResult::AlreadyExists);
    EOKAS_EXPECT(vfs.attach("/blocked/child", std::make_unique<VfsMemoryVolume>(), true) == VfsResult::AlreadyExists);

    EOKAS_EXPECT(vfs.attach("/data", std::make_unique<VfsMemoryVolume>(), false) == VfsResult::Ok);
    EOKAS_EXPECT(vfs.writeText("/data/a.txt", "no") == VfsResult::Access);
    EOKAS_EXPECT(vfs.remove("/data") == VfsResult::Access);
    EOKAS_EXPECT(vfs.readText("/data/a.txt", text) == VfsResult::NotFound);
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
        Vfs vfs;
        EOKAS_EXPECT(vfs.attach("/", std::make_unique<VfsMemoryVolume>(), true) == VfsResult::Ok);
        EOKAS_EXPECT(vfs.writeText("/a.txt", "hi") == VfsResult::Ok);
        EOKAS_EXPECT(vfs.open("/a.txt", VfsMode::Read, kept) == VfsResult::Ok);
    }
    String text;
    EOKAS_EXPECT(kept.readText(text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "hi");
    EOKAS_EXPECT(kept.close() == VfsResult::Ok);

    auto memory = std::make_unique<VfsMemoryVolume>();
    VfsMemoryVolume* raw = memory.get();
    Vfs vfs;
    EOKAS_EXPECT(vfs.attach("/", std::move(memory), true) == VfsResult::Ok);
    EOKAS_EXPECT(vfs.writeText("/a.txt", "hi") == VfsResult::Ok);
    VfsFile file;
    EOKAS_EXPECT(vfs.open("/a.txt", VfsMode::ReadWrite, file) == VfsResult::Ok);
    EOKAS_EXPECT(vfs.detach("/") == VfsResult::Ok);
    bool present = true;
    EOKAS_EXPECT(vfs.exists("/a.txt", present) == VfsResult::Ok);
    EOKAS_EXPECT(!present);
    EOKAS_EXPECT(file.readText(text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "hi");
    EOKAS_EXPECT(file.writeText("yo") == VfsResult::Ok);
    EOKAS_EXPECT(file.flush() == VfsResult::Ok);
    EOKAS_EXPECT(file.readText(text) == VfsResult::Ok);
    EOKAS_EXPECT(text == "yo");
    String manifest = String("m/") + VfsPath::hash("/a.txt");
    EOKAS_EXPECT(raw->exists(manifest, present) == VfsResult::Ok);
    EOKAS_EXPECT(present);
    EOKAS_EXPECT(file.close() == VfsResult::Ok);
    EOKAS_EXPECT(vfs.open("/a.txt", VfsMode::Read, file) == VfsResult::NotFound);
    return 0;
}

EOKAS_TEST_CASE(vfsstream) {
    auto memory = std::make_unique<VfsMemoryVolume>();
    VfsMemoryVolume* raw = memory.get();
    Vfs vfs;
    EOKAS_EXPECT(vfs.attach("/", std::move(memory), true) == VfsResult::Ok);

    std::vector<u8_t> body(VfsChunkBytes * 2, 'a');
    body[VfsChunkBytes] = 'b';
    EOKAS_EXPECT(vfs.writeAll("/wide.bin", body.data(), body.size()) == VfsResult::Ok);
    EOKAS_EXPECT(keyCount(*raw, "c/") == 2);

    VfsFile file;
    u64_t count = 0;
    EOKAS_EXPECT(vfs.open("/wide.bin", VfsMode::ReadWrite, file) == VfsResult::Ok);
    EOKAS_EXPECT(file.write("Z", 1, count) == VfsResult::Ok);
    EOKAS_EXPECT(file.seek(0, VfsSeek::Begin) == VfsResult::Ok);
    char head[2] = {};
    EOKAS_EXPECT(file.read(head, 2, count) == VfsResult::Ok);
    EOKAS_EXPECT(head[0] == 'Z' && head[1] == 'a');
    EOKAS_EXPECT(file.seek((i64_t) VfsChunkBytes, VfsSeek::Begin) == VfsResult::Ok);
    char mid = 0;
    EOKAS_EXPECT(file.read(&mid, 1, count) == VfsResult::Ok);
    EOKAS_EXPECT(mid == 'b');
    EOKAS_EXPECT(file.close() == VfsResult::Ok);

    std::vector<u8_t> out;
    EOKAS_EXPECT(vfs.readAll("/wide.bin", out) == VfsResult::Ok);
    EOKAS_EXPECT(out.size() == body.size());
    EOKAS_EXPECT(out[0] == 'Z' && out[1] == 'a' && out[VfsChunkBytes] == 'b');
    VfsStat wideStat;
    EOKAS_EXPECT(vfs.stat("/wide.bin", wideStat) == VfsResult::Ok);
    EOKAS_EXPECT(wideStat.hash == vfsHash(out.data(), out.size()));

    VfsFile gap;
    EOKAS_EXPECT(vfs.open("/gap.bin", VfsMode::Write, gap) == VfsResult::Ok);
    EOKAS_EXPECT(gap.seek((i64_t) VfsChunkBytes * 2, VfsSeek::Begin) == VfsResult::Ok);
    EOKAS_EXPECT(gap.write("Q", 1, count) == VfsResult::Ok);
    EOKAS_EXPECT(gap.close() == VfsResult::Ok);
    EOKAS_EXPECT(vfs.readAll("/gap.bin", out) == VfsResult::Ok);
    EOKAS_EXPECT(out.size() == VfsChunkBytes * 2 + 1);
    EOKAS_EXPECT(out[0] == 0 && out[VfsChunkBytes] == 0 && out.back() == 'Q');
    VfsStat gapStat;
    EOKAS_EXPECT(vfs.stat("/gap.bin", gapStat) == VfsResult::Ok);
    EOKAS_EXPECT(gapStat.hash == vfsHash(out.data(), out.size()));
    std::vector<u8_t> zeros(VfsChunkBytes, 0);
    String zeroHash = vfsHash(zeros.data(), zeros.size());
    EOKAS_EXPECT(refIs(*raw, zeroHash, 2));
    return 0;
}
