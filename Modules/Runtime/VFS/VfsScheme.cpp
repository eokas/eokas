#include "VfsScheme.h"

namespace eokas
{
    VfsResult VfsScheme::bind(VfsFile& file, std::unique_ptr<VfsSchemeFile> body)
    {
        return file.openScheme(std::move(body));
    }
}
