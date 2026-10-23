#include "VfsHash.h"
#include "Base/Hash.h"

namespace eokas
{
    String vfsHash(const void* data, u64_t size)
    {
        const u8_t* bytes = static_cast<const u8_t*>(data);
        return SHA256().compute(bytes, (size_t) size);
    }
}
