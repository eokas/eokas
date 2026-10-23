#ifndef _EOKAS_VFS_RESULT_H_
#define _EOKAS_VFS_RESULT_H_

#include "Base/String.h"

namespace eokas
{
    enum class VfsResult
    {
        Ok,
        InvalidPath,
        NotFound,
        AlreadyExists,
        NotEmpty,
        Access,
        Io,
        Corrupt,
        Mismatch,
        Unsupported,
        Closed
    };
}

#endif//_EOKAS_VFS_RESULT_H_
