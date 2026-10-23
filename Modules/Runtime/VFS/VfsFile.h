#ifndef _EOKAS_VFS_FILE_H_
#define _EOKAS_VFS_FILE_H_

#include "VfsResult.h"
#include <memory>

namespace eokas
{
    enum class VfsMode
    {
        Read,
        Write,
        ReadWrite,
        Append
    };

    enum class VfsSeek
    {
        Begin,
        Current,
        End
    };

    class VfsChunkStore;
    struct VfsManifest;

    class VfsFile
    {
    public:
        VfsFile();
        ~VfsFile();

        VfsFile(const VfsFile&) = delete;
        VfsFile& operator=(const VfsFile&) = delete;
        VfsFile(VfsFile&& other) noexcept;
        VfsFile& operator=(VfsFile&& other) noexcept;

        VfsResult read(void* data, u64_t size, u64_t& count);
        VfsResult write(const void* data, u64_t size, u64_t& count);
        VfsResult seek(i64_t offset, VfsSeek origin);
        VfsResult flush();
        VfsResult close();
        VfsResult readText(String& text);
        VfsResult writeText(const String& text);
        u64_t tell() const;
        u64_t size() const;
        bool isOpen() const;

    private:
        friend class Vfs;

        struct State;
        std::unique_ptr<State> mState;

        VfsResult prepare(std::shared_ptr<VfsChunkStore> readStore, std::shared_ptr<VfsChunkStore> writeStore, const VfsManifest& manifest, const VfsManifest* baseline, VfsMode mode, bool materialized, bool dirty, u64_t pos);
        static VfsResult spillResident(State& state);
        static VfsResult ensureResident(State& state, size_t index);
        static VfsResult growZero(State& state, u64_t target);
    };
}

#endif//_EOKAS_VFS_FILE_H_
