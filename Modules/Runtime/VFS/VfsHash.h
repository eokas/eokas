#ifndef _EOKAS_VFS_HASH_H_
#define _EOKAS_VFS_HASH_H_

#include "Base/String.h"

namespace eokas
{
    const u32_t VfsChunkBytes = 65536;

    String vfsHash(const void* data, u64_t size);
}

#endif//_EOKAS_VFS_HASH_H_
