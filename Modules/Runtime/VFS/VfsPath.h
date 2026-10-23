#ifndef _EOKAS_VFS_PATH_H_
#define _EOKAS_VFS_PATH_H_

#include "VfsResult.h"

namespace eokas
{
    class VfsPath
    {
    public:
        static VfsResult normalize(const String& in, String& out);
        static String parent(const String& path);
        static String name(const String& path);
        static String join(const String& dir, const String& name);
        static String hash(const String& normalizedPath);
    };
}

#endif//_EOKAS_VFS_PATH_H_
