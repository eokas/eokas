#include "VfsMemoryVolume.h"

namespace eokas
{
    VfsResult VfsMemoryVolume::exists(const String& key, bool& out)
    {
        out = mBlobs.find(key) != mBlobs.end();
        return VfsResult::Ok;
    }

    VfsResult VfsMemoryVolume::get(const String& key, std::vector<u8_t>& out)
    {
        auto found = mBlobs.find(key);
        if (found == mBlobs.end())
            return VfsResult::NotFound;
        out = found->second;
        return VfsResult::Ok;
    }

    VfsResult VfsMemoryVolume::put(const String& key, const void* data, u64_t size)
    {
        std::vector<u8_t> bytes;
        if (size > 0 && data != nullptr)
        {
            const u8_t* src = static_cast<const u8_t*>(data);
            bytes.assign(src, src + (size_t) size);
        }
        mBlobs[key] = std::move(bytes);
        return VfsResult::Ok;
    }

    VfsResult VfsMemoryVolume::remove(const String& key)
    {
        auto found = mBlobs.find(key);
        if (found == mBlobs.end())
            return VfsResult::NotFound;
        mBlobs.erase(found);
        return VfsResult::Ok;
    }

    VfsResult VfsMemoryVolume::list(const String& prefix, std::vector<String>& out)
    {
        out.clear();
        for (auto& item : mBlobs)
        {
            if (item.first.startsWith(prefix))
                out.push_back(item.first);
        }
        return VfsResult::Ok;
    }
}
