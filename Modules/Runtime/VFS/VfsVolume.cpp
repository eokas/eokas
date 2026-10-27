#include "VfsVolume.h"
#include "VfsScheme.h"

namespace eokas
{
    VfsVolume::VfsVolume() = default;

    VfsVolume::~VfsVolume() = default;

    void VfsVolume::setScheme(std::unique_ptr<VfsScheme> scheme)
    {
        mScheme = std::move(scheme);
    }

    void VfsVolume::setWritable(bool writable)
    {
        mWritable = writable;
    }

    std::unique_ptr<VfsScheme> VfsVolume::takeScheme()
    {
        return std::move(mScheme);
    }

    bool VfsVolume::writable() const
    {
        return mWritable;
    }
}
