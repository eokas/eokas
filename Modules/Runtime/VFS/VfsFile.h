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

    class VfsScheme;
    class VfsSchemeFile;

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
        friend class VfsScheme;

        std::unique_ptr<VfsSchemeFile> mBody;

        VfsResult openScheme(std::unique_ptr<VfsSchemeFile> file);
    };
}

#endif//_EOKAS_VFS_FILE_H_
