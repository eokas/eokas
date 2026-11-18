#ifndef _EOKAS_ROSE_AXION_INSTANCE_KIND_H_
#define _EOKAS_ROSE_AXION_INSTANCE_KIND_H_

#include "Rose/Schema.h"

namespace eokas::RoseAxion {

    enum class RoseInstanceKind
    {
        Int,
        Float,
        Text,
        Url,
        Email,
        DateTime,
        Color,
        Options,
        Struct,
        ListOptions,
        ListElement,
        Boolean,
        FilePath,
        FolderPath,
        Unsupported,
    };

    bool isOptionsSchemaType(const rose::Schema* schema);
    bool isMultiOptionsListSchema(const rose::Schema* schema);
    RoseInstanceKind classifyInstanceKind(const rose::Schema* schema);

}

#endif