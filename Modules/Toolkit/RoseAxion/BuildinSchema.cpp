#include "./BuildinSchema.h"

namespace eokas::RoseAxion {

    namespace {

        const BuildinSchema kBuildinSchemas[] = {
            { rose::SchemaType::Int, "Int" },
            { rose::SchemaType::Float, "Float" },
            { rose::SchemaType::Bool, "Bool" },
            { rose::SchemaType::String, "String" },
            { rose::SchemaType::String, "Date" },
            { rose::SchemaType::String, "Time" },
            { rose::SchemaType::String, "DateTime" },
            { rose::SchemaType::String, "URL" },
            { rose::SchemaType::String, "Email" },
            { rose::SchemaType::String, "Color" },
            { rose::SchemaType::String, "FilePath" },
            { rose::SchemaType::String, "FolderPath" },
        };

        const u32_t kBuildinSchemaCount = static_cast<u32_t>(sizeof(kBuildinSchemas) / sizeof(kBuildinSchemas[0]));

    }

    bool isBuildinSchema(const String& name)
    {
        for (u32_t i = 0; i < kBuildinSchemaCount; ++i)
        {
            if (name == kBuildinSchemas[i].name)
            {
                return true;
            }
        }
        return false;
    }

    u32_t buildinSchemaCount()
    {
        return kBuildinSchemaCount;
    }

    const char* buildinSchemaName(u32_t index)
    {
        if (index >= kBuildinSchemaCount)
        {
            return "";
        }
        return kBuildinSchemas[index].name;
    }

    rose::SchemaType buildinSchemaType(u32_t index)
    {
        if (index >= kBuildinSchemaCount)
        {
            return rose::SchemaType::None;
        }
        return kBuildinSchemas[index].type;
    }

    bool ensureBuildinSchemas(rose::Library& library)
    {
        for (u32_t i = 0; i < kBuildinSchemaCount; ++i)
        {
            const BuildinSchema& item = kBuildinSchemas[i];
            rose::Schema* schema = library.getSchema(item.name);
            if (schema == nullptr)
            {
                schema = library.addSchema(item.type, String(item.name));
            }
            if (schema == nullptr || schema->type() != item.type)
            {
                return false;
            }
        }
        return true;
    }

}
