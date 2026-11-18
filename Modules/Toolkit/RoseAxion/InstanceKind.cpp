#include "./InstanceKind.h"
#include "./FieldTypes.h"

namespace eokas::RoseAxion {

    bool isOptionsSchemaType(const rose::Schema* schema)
    {
        return schema != nullptr && schema->is(rose::SchemaType::Options);
    }

    bool isMultiOptionsListSchema(const rose::Schema* schema)
    {
        return schema != nullptr
            && schema->is(rose::SchemaType::List)
            && isOptionsSchemaType(schema->getElementSchema());
    }

    RoseInstanceKind classifyInstanceKind(const rose::Schema* schema)
    {
        if (schema == nullptr)
        {
            return RoseInstanceKind::Unsupported;
        }
        if (schema->is(rose::SchemaType::Struct))
        {
            return RoseInstanceKind::Struct;
        }
        if (isMultiOptionsListSchema(schema))
        {
            return RoseInstanceKind::ListOptions;
        }
        if (schema->is(rose::SchemaType::List))
        {
            return RoseInstanceKind::ListElement;
        }
        if (isOptionsSchemaType(schema))
        {
            return RoseInstanceKind::Options;
        }
        if (schema->is(rose::SchemaType::Int))
        {
            return RoseInstanceKind::Int;
        }
        if (schema->is(rose::SchemaType::Float))
        {
            return RoseInstanceKind::Float;
        }
        if (schema->is(rose::SchemaType::Bool))
        {
            return RoseInstanceKind::Boolean;
        }
        FieldKind kind = fieldKindFromSchemaName(schema->name());
        switch (kind)
        {
        case FieldKind::Url:
            return RoseInstanceKind::Url;
        case FieldKind::Email:
            return RoseInstanceKind::Email;
        case FieldKind::Date:
        case FieldKind::Time:
        case FieldKind::DateTime:
            return RoseInstanceKind::DateTime;
        case FieldKind::Color:
            return RoseInstanceKind::Color;
        case FieldKind::FilePath:
            return RoseInstanceKind::FilePath;
        case FieldKind::FolderPath:
            return RoseInstanceKind::FolderPath;
        case FieldKind::Integer:
            return RoseInstanceKind::Int;
        case FieldKind::Float:
            return RoseInstanceKind::Float;
        case FieldKind::Boolean:
            return RoseInstanceKind::Boolean;
        default:
            return RoseInstanceKind::Text;
        }
    }

}