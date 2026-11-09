#ifndef _EOKAS_ROSE_AXION_BUILDIN_SCHEMA_H_
#define _EOKAS_ROSE_AXION_BUILDIN_SCHEMA_H_

#include "./Header.h"

#include "Rose/Library.h"

namespace eokas::RoseAxion {

    struct BuildinSchema
    {
        rose::SchemaType type;
        const char* name;
    };

    bool isBuildinSchema(const String& name);
    u32_t buildinSchemaCount();
    const char* buildinSchemaName(u32_t index);
    rose::SchemaType buildinSchemaType(u32_t index);
    bool ensureBuildinSchemas(rose::Library& library);

}

#endif
