#include "VfsFile.h"
#include "VfsScheme.h"
#include <cstring>
#include <vector>

namespace eokas
{
    VfsFile::VfsFile()
        : mBody(nullptr)
    {
    }

    VfsFile::~VfsFile()
    {
        if (close() != VfsResult::Ok)
            mBody.reset();
    }

    VfsFile::VfsFile(VfsFile&& other) noexcept
        : mBody(std::move(other.mBody))
    {
    }

    VfsFile& VfsFile::operator=(VfsFile&& other) noexcept
    {
        if (this != &other)
        {
            if (close() != VfsResult::Ok)
                mBody.reset();
            mBody = std::move(other.mBody);
        }
        return *this;
    }

    bool VfsFile::isOpen() const
    {
        return mBody != nullptr && mBody->isOpen();
    }

    u64_t VfsFile::tell() const
    {
        if (!isOpen())
            return 0;
        return mBody->tell();
    }

    u64_t VfsFile::size() const
    {
        if (!isOpen())
            return 0;
        return mBody->size();
    }

    VfsResult VfsFile::openScheme(std::unique_ptr<VfsSchemeFile> file)
    {
        VfsResult closed = close();
        if (closed != VfsResult::Ok)
            return closed;
        mBody = std::move(file);
        return VfsResult::Ok;
    }

    VfsResult VfsFile::read(void* data, u64_t size, u64_t& count)
    {
        if (!isOpen())
            return VfsResult::Closed;
        return mBody->read(data, size, count);
    }

    VfsResult VfsFile::write(const void* data, u64_t size, u64_t& count)
    {
        if (!isOpen())
            return VfsResult::Closed;
        return mBody->write(data, size, count);
    }

    VfsResult VfsFile::seek(i64_t offset, VfsSeek origin)
    {
        if (!isOpen())
            return VfsResult::Closed;
        return mBody->seek(offset, origin);
    }

    VfsResult VfsFile::flush()
    {
        if (!isOpen())
            return VfsResult::Closed;
        return mBody->flush();
    }

    VfsResult VfsFile::close()
    {
        if (mBody == nullptr)
            return VfsResult::Ok;
        VfsResult result = mBody->close();
        if (result != VfsResult::Ok)
            return result;
        mBody.reset();
        return VfsResult::Ok;
    }

    VfsResult VfsFile::readText(String& text)
    {
        if (!isOpen())
            return VfsResult::Closed;
        u64_t saved = tell();
        VfsResult result = seek(0, VfsSeek::Begin);
        if (result != VfsResult::Ok)
            return result;
        u64_t fileSize = size();
        std::vector<u8_t> bytes((size_t) fileSize, 0);
        u64_t count = 0;
        if (fileSize > 0)
        {
            result = read(bytes.data(), fileSize, count);
            if (result != VfsResult::Ok)
                return result;
        }
        seek((i64_t) saved, VfsSeek::Begin);
        for (u64_t index = 0; index < count; ++index)
        {
            if (bytes[(size_t) index] == 0)
                return VfsResult::Corrupt;
        }
        std::vector<char> chars((size_t) count + 1, '\0');
        if (count > 0)
            std::memcpy(chars.data(), bytes.data(), (size_t) count);
        text = String(chars.data());
        return VfsResult::Ok;
    }

    VfsResult VfsFile::writeText(const String& text)
    {
        if (!isOpen())
            return VfsResult::Closed;
        if (mBody->mode() == VfsMode::Read)
            return VfsResult::Access;
        u64_t count = 0;
        return write(text.cstr(), text.length(), count);
    }
}
