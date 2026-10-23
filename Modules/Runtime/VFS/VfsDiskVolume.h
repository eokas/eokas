#ifndef _EOKAS_VFS_DISK_VOLUME_H_
#define _EOKAS_VFS_DISK_VOLUME_H_

#include "VfsVolume.h"

namespace eokas
{
    class VfsDiskVolume : public VfsVolume
    {
    public:
        explicit VfsDiskVolume(const String& root);

        VfsResult exists(const String& key, bool& out) override;
        VfsResult get(const String& key, std::vector<u8_t>& out) override;
        VfsResult put(const String& key, const void* data, u64_t size) override;
        VfsResult remove(const String& key) override;
        VfsResult list(const String& prefix, std::vector<String>& out) override;

    private:
        String mRoot;
    };
}

#endif//_EOKAS_VFS_DISK_VOLUME_H_
