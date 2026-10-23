#ifndef _EOKAS_VFS_VOLUME_H_
#define _EOKAS_VFS_VOLUME_H_

#include "VfsResult.h"
#include <vector>

namespace eokas
{
    class VfsVolume
    {
    public:
        virtual ~VfsVolume() = default;

        virtual VfsResult exists(const String& key, bool& out) = 0;
        virtual VfsResult get(const String& key, std::vector<u8_t>& out) = 0;
        virtual VfsResult put(const String& key, const void* data, u64_t size) = 0;
        virtual VfsResult remove(const String& key) = 0;
        virtual VfsResult list(const String& prefix, std::vector<String>& out) = 0;
    };
}

#endif//_EOKAS_VFS_VOLUME_H_
