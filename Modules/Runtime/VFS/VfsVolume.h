#ifndef _EOKAS_VFS_VOLUME_H_
#define _EOKAS_VFS_VOLUME_H_

#include "VfsResult.h"
#include <memory>
#include <vector>

namespace eokas
{
    class VfsScheme;

    class VfsVolume
    {
    public:
        VfsVolume();
        virtual ~VfsVolume();

        void setScheme(std::unique_ptr<VfsScheme> scheme);
        void setWritable(bool writable);

        virtual VfsResult exists(const String& key, bool& out) = 0;
        virtual VfsResult get(const String& key, std::vector<u8_t>& out) = 0;
        virtual VfsResult put(const String& key, const void* data, u64_t size) = 0;
        virtual VfsResult remove(const String& key) = 0;
        virtual VfsResult list(const String& prefix, std::vector<String>& out) = 0;

    private:
        friend class VfsRoot;

        std::unique_ptr<VfsScheme> mScheme;
        bool mWritable = false;

        std::unique_ptr<VfsScheme> takeScheme();
        bool writable() const;
    };
}

#endif//_EOKAS_VFS_VOLUME_H_
