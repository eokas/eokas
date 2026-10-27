#ifndef _EOKAS_VFS_BLOCK_SCHEME_H_
#define _EOKAS_VFS_BLOCK_SCHEME_H_

#include "VfsScheme.h"
#include <map>
#include <vector>

namespace eokas
{
    class VfsBlockScheme;

    class VfsBlockSchemeFile : public VfsSchemeFile
    {
    public:
        VfsBlockSchemeFile(std::shared_ptr<VfsBlockScheme> block, u64_t ino, VfsMode mode, u64_t size, u32_t blockSize, u64_t pos);
        ~VfsBlockSchemeFile() override;

        VfsBlockSchemeFile(const VfsBlockSchemeFile&) = delete;
        VfsBlockSchemeFile& operator=(const VfsBlockSchemeFile&) = delete;

        VfsResult read(void* data, u64_t size, u64_t& count) override;
        VfsResult write(const void* data, u64_t size, u64_t& count) override;
        VfsResult seek(i64_t offset, VfsSeek origin) override;
        VfsResult flush() override;
        VfsResult close() override;
        u64_t tell() const override;
        u64_t size() const override;
        bool isOpen() const override;
        VfsMode mode() const override;

    private:
        std::shared_ptr<VfsBlockScheme> mBlock;
        u64_t mIno = 0;
        VfsMode mMode = VfsMode::Read;
        u64_t mSize = 0;
        u32_t mBlockSize = 0;
        u64_t mPos = 0;
        bool mOpen = false;
        bool mValid = false;
        bool mDirty = false;
        bool mMetaDirty = false;
        u64_t mIndex = 0;
        std::vector<u8_t> mBuf;

        VfsResult load(u64_t index);
        VfsResult store();
    };

    class VfsBlockScheme : public VfsScheme
    {
    public:
        VfsBlockScheme();
        VfsBlockScheme(const VfsBlockScheme&) = delete;
        VfsBlockScheme& operator=(const VfsBlockScheme&) = delete;

        VfsResult stat(const String& path, VfsKind& kind, u64_t& size, i64_t& mtime) override;
        VfsResult exists(const String& path, bool& out) override;
        VfsResult list(const String& path, std::vector<VfsDirEntry>& out) override;
        VfsResult createDir(const String& path) override;
        VfsResult remove(const String& path) override;
        VfsResult rename(const String& from, const String& to) override;
        VfsResult link(const String& existing, const String& path) override;
        VfsResult openFile(const String& path, VfsMode mode, VfsFile& file) override;
        VfsResult open(std::shared_ptr<VfsVolume> volume, bool writable) override;

    private:
        friend class VfsBlockSchemeFile;

        struct Node
        {
            u64_t ino = 0;
            u64_t parent = 0;
            String name;
            VfsKind kind = VfsKind::File;
            u32_t nlink = 0;
            u64_t size = 0;
            i64_t mtime = 0;
        };

        struct DirItem
        {
            String name;
            u64_t ino = 0;
            VfsKind kind = VfsKind::File;
        };

        std::shared_ptr<VfsVolume> mVolume;
        u32_t mBlockSize = 65536;
        u64_t mNextIno = 2;
        std::vector<u64_t> mOrphans;
        std::map<u64_t, u32_t> mOpen;

        VfsResult format();
        VfsResult replay();
        bool decodeSuper(const std::vector<u8_t>& bytes);
        VfsResult writeSuper();
        VfsResult readInode(u64_t ino, Node& out);
        VfsResult writeInode(const Node& node);
        VfsResult readDir(u64_t ino, std::vector<DirItem>& out);
        VfsResult writeDir(u64_t ino, const std::vector<DirItem>& entries);
        VfsResult lookup(const String& path, Node& out);
        VfsResult ensureDir(const String& path);
        VfsResult createNode(const String& path, VfsKind kind, Node& out);
        VfsResult allocIno(u64_t& ino);
        VfsResult addOrphan(u64_t ino);
        VfsResult removeOrphan(u64_t ino);
        VfsResult linkEntry(u64_t parent, const String& name, u64_t ino, VfsKind kind);
        VfsResult unlinkEntry(u64_t parent, const String& name);
        VfsResult forgetEntry(u64_t parent, const String& name);
        VfsResult moveEntry(u64_t srcParent, const String& srcName, u64_t dstParent, const String& dstName);
        VfsResult truncate(u64_t ino);
        VfsResult freeInode(u64_t ino);
        void noteOpen(u64_t ino);
        void noteClose(u64_t ino);
        VfsResult readBlock(u64_t ino, u64_t index, std::vector<u8_t>& out);
        VfsResult writeBlock(u64_t ino, u64_t index, const void* data, u64_t size);
        VfsResult dropBlock(u64_t ino, u64_t index);
        VfsResult updateFile(u64_t ino, u64_t size);
    };
}

#endif//_EOKAS_VFS_BLOCK_SCHEME_H_
