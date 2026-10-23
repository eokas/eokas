#ifndef _EOKAS_VFS_MEMORY_VOLUME_H_
#define _EOKAS_VFS_MEMORY_VOLUME_H_

#include "VfsVolume.h"
#include <map>

namespace eokas
{
    class VfsMemoryVolume : public VfsVolume
    {
    public:
        VfsResult exists(const String& key, bool& out) override;
        VfsResult get(const String& key, std::vector<u8_t>& out) override;
        VfsResult put(const String& key, const void* data, u64_t size) override;
        VfsResult remove(const String& key) override;
        VfsResult list(const String& prefix, std::vector<String>& out) override;

    private:
        std::map<String, std::vector<u8_t>> mBlobs;
    };
}

#endif//_EOKAS_VFS_MEMORY_VOLUME_H_
