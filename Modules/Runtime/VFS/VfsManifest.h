#ifndef _EOKAS_VFS_MANIFEST_H_
#define _EOKAS_VFS_MANIFEST_H_

#include "VfsResult.h"

namespace eokas
{
    enum class VfsKind : u8_t
    {
        File = 1,
        Dir = 2
    };

    struct VfsDirEntry
    {
        String name;
        VfsKind kind;
    };
}

#endif//_EOKAS_VFS_MANIFEST_H_
