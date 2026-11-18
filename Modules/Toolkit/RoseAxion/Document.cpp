#include "./Document.h"
#include "./BuildinSchema.h"

#include "Base/IO.h"

#include <functional>
#include <set>
#include <stdexcept>
#include <utility>

namespace eokas::RoseAxion {

    using rose::Schema;
    using rose::Value;

    namespace {

        u32_t schemaColorFromHex(const String& hex, u32_t fallback = 0xff6366f1u)
        {
            Color color;
            if (hex.isEmpty() || !parseColor(hex, color))
            {
                return fallback;
            }
            u32_t r = static_cast<u32_t>(color.r * 255.0f + 0.5f);
            u32_t g = static_cast<u32_t>(color.g * 255.0f + 0.5f);
            u32_t b = static_cast<u32_t>(color.b * 255.0f + 0.5f);
            u32_t a = static_cast<u32_t>(color.a * 255.0f + 0.5f);
            return r | (g << 8) | (b << 16) | (a << 24);
        }

        String hexFromSchemaColor(u32_t value)
        {
            Color color(
                static_cast<float>(value & 0xff) / 255.0f,
                static_cast<float>((value >> 8) & 0xff) / 255.0f,
                static_cast<float>((value >> 16) & 0xff) / 255.0f,
                static_cast<float>((value >> 24) & 0xff) / 255.0f);
            return formatColor(color);
        }

        bool optionNameTaken(const std::vector<String>& taken, const String& name)
        {
            for (const String& existing : taken)
            {
                if (existing == name)
                {
                    return true;
                }
            }
            return false;
        }

        String uniquePreparedOptionName(const std::vector<String>& taken, const char* prefix)
        {
            u32_t number = static_cast<u32_t>(taken.size()) + 1;
            for (;;)
            {
                String name = String::format("%s%u", prefix, number);
                if (!optionNameTaken(taken, name))
                {
                    return name;
                }
                ++number;
            }
        }

        bool optionNameConflict(const std::vector<FieldOption>& options)
        {
            std::vector<String> seen;
            for (const FieldOption& option : options)
            {
                String name = option.name.trim();
                if (name.isEmpty())
                {
                    continue;
                }
                if (optionNameTaken(seen, name))
                {
                    return true;
                }
                seen.push_back(name);
            }
            return false;
        }

        bool prepareSchemaOptions(const std::vector<FieldOption>& options, std::vector<FieldOption>& prepared)
        {
            prepared.clear();
            std::vector<String> taken;
            for (const FieldOption& option : options)
            {
                String name = option.name.trim();
                String label = option.label;
                if (name.isEmpty() && label.trim().isEmpty())
                {
                    continue;
                }
                if (name.isEmpty())
                {
                    name = uniquePreparedOptionName(taken, "opt_");
                }
                else if (optionNameTaken(taken, name))
                {
                    return false;
                }
                FieldOption item;
                item.name = name;
                item.label = label;
                item.color = option.color;
                taken.push_back(name);
                prepared.push_back(item);
            }
            return true;
        }

        String optionStorageName(const FieldOption& option)
        {
            if (!option.name.isEmpty())
            {
                return option.name;
            }
            return option.label;
        }

        String optionDisplayLabel(const std::vector<FieldOption>& options, const String& stored)
        {
            for (const FieldOption& option : options)
            {
                if (option.name == stored)
                {
                    return option.label.isEmpty() ? option.name : option.label;
                }
            }
            return stored;
        }

        String optionStorageNameFromLabel(const std::vector<FieldOption>& options, const String& labelOrName)
        {
            for (const FieldOption& option : options)
            {
                if (option.name == labelOrName
                    || (!option.label.isEmpty() && option.label == labelOrName))
                {
                    return optionStorageName(option);
                }
            }
            return String::empty;
        }

        bool isOptionsSchema(Schema* schema)
        {
            return schema != nullptr && schema->is(rose::SchemaType::Options);
        }

        bool isMultiOptionsList(Schema* schema)
        {
            return schema != nullptr
                && schema->is(rose::SchemaType::List)
                && isOptionsSchema(schema->getElementSchema());
        }

        Schema* optionsSchemaOf(Schema* storage)
        {
            if (isOptionsSchema(storage))
            {
                return storage;
            }
            if (isMultiOptionsList(storage))
            {
                return storage->getElementSchema();
            }
            return nullptr;
        }

        std::vector<FieldOption> fieldOptionsFromSchema(Schema* schema)
        {
            std::vector<FieldOption> out;
            if (!isOptionsSchema(schema))
            {
                return out;
            }
            u32_t count = schema->getOptionCount();
            for (u32_t index = 0; index < count; ++index)
            {
                Schema::Option* option = schema->getOption(index);
                if (option == nullptr)
                {
                    continue;
                }
                FieldOption item;
                item.name = option->name;
                item.label = option->name;
                item.color = hexFromSchemaColor(option->color);
                out.push_back(item);
            }
            return out;
        }

        bool replaceSchemaOptions(Schema* schema, const std::vector<FieldOption>& options)
        {
            if (!isOptionsSchema(schema))
            {
                return false;
            }
            std::vector<FieldOption> prepared;
            if (!prepareSchemaOptions(options, prepared))
            {
                return false;
            }
            while (schema->getOptionCount() > 0)
            {
                Schema::Option* existing = schema->getOption(0);
                if (existing == nullptr)
                {
                    break;
                }
                schema->delOption(existing->name);
            }
            for (const FieldOption& option : prepared)
            {
                if (!schema->addOption(option.name, schemaColorFromHex(option.color)))
                {
                    return false;
                }
            }
            return true;
        }

    }

    const char* docStatusText(DocStatus status)
    {
        switch (status)
        {
        case DocStatus::Opened: return "Opened";
        case DocStatus::Saved: return "Saved";
        case DocStatus::OpenFailed: return "Failed to open";
        case DocStatus::CannotOpenFile: return "Cannot open file";
        case DocStatus::InvalidLibrary: return "Unrecognized Rose Library";
        case DocStatus::SaveFailed: return "Failed to save";
        case DocStatus::CannotSaveFile: return "Cannot save file";
        case DocStatus::SchemaNameEmpty: return "Schema name cannot be empty";
        case DocStatus::SchemaNameTaken: return "Schema name already exists";
        case DocStatus::CannotCreateSchema: return "Cannot create schema";
        case DocStatus::CannotDeleteSchema: return "Cannot delete schema";
        case DocStatus::CannotAddMember: return "Cannot add member";
        case DocStatus::MemberNameEmpty: return "Member name cannot be empty";
        case DocStatus::MemberNameTaken: return "Member name already exists";
        case DocStatus::OptionNameEmpty: return "Option name cannot be empty";
        case DocStatus::OptionNameTaken: return "Option name already exists";
        case DocStatus::CannotUpdateOptions: return "Cannot update options";
        case DocStatus::InstanceNameEmpty: return "Instance name cannot be empty";
        case DocStatus::InstanceNameTaken: return "Instance name already exists";
        case DocStatus::CannotCreateInstance: return "Cannot create instance";
        case DocStatus::CannotDeleteInstance: return "Cannot delete instance";
        case DocStatus::TypeMismatchCleared: return "Content did not match field type; cleared";
        case DocStatus::Ready:
        default:
            return "";
        }
    }

    bool docStatusFailed(DocStatus status)
    {
        switch (status)
        {
        case DocStatus::Ready:
        case DocStatus::Opened:
        case DocStatus::Saved:
            return false;
        default:
            return true;
        }
    }

    void TableDocument::markDirty()
    {
        mDirty = true;
    }

    void TableDocument::createNew()
    {
        mLibrary = std::make_unique<rose::Library>("table");
        this->clearActive();
        mInstances = nullptr;
        if (!ensureBuildinSchemas(*mLibrary) || !this->ensureSharedSchemas(true))
        {
            mLibrary.reset();
            mOpen = false;
            this->setStatus(DocStatus::CannotCreateInstance);
            return;
        }
        if (!this->ensureInstancesRegistry())
        {
            mLibrary.reset();
            mOpen = false;
            this->setStatus(DocStatus::CannotCreateInstance);
            return;
        }
        mPath = String::empty;
        mOpen = true;
        mDirty = false;
        mLoadedLibraryBytes = 0;
        this->setStatus(DocStatus::Ready);
    }

    String TableDocument::librarySizeText() const
    {
        return String::format("Size: %.3f KB", static_cast<double>(mLoadedLibraryBytes) / 1024.0);
    }

    bool TableDocument::load(const String& path)
    {
        auto next = std::make_unique<rose::Library>("table");
        if (!next->load(path))
        {
            this->setStatus(DocStatus::OpenFailed);
            return false;
        }
        auto previous = std::move(mLibrary);
        mLibrary = std::move(next);
        if (!this->bind())
        {
            mLibrary = std::move(previous);
            if (mLibrary != nullptr)
            {
                this->bind();
            }
            else
            {
                this->clearActive();
                mInstances = nullptr;
                mOpen = false;
            }
            this->setStatus(DocStatus::InvalidLibrary);
            return false;
        }
        mPath = path;
        size_t bytes = 0;
        FileStream probe(path, "rb");
        if (probe.open())
        {
            bytes = probe.size();
            probe.close();
        }
        mLoadedLibraryBytes = bytes;
        this->setStatus(DocStatus::Opened);
        return true;
    }

    bool TableDocument::save(const String& path)
    {
        if (!mOpen || mLibrary == nullptr)
        {
            return false;
        }
        if (!mLibrary->save(path))
        {
            this->setStatus(DocStatus::SaveFailed);
            return false;
        }
        mPath = path;
        mDirty = false;
        this->setStatus(DocStatus::Saved);
        return true;
    }

    bool TableDocument::bind()
    {
        mOpen = false;
        this->clearActive();
        mInstances = nullptr;
        if (mLibrary == nullptr)
        {
            return false;
        }
        if (!this->ensureSharedSchemas(false))
        {
            return false;
        }
        if (!this->ensureInstancesRegistry())
        {
            return false;
        }
        mOpen = true;
        mDirty = false;
        if (this->instanceCount() > 0 && mActiveInstance < 0)
        {
            mActiveInstance = 0;
        }
        return true;
    }

    bool TableDocument::ensureSharedSchemas(bool install)
    {
        if (mLibrary == nullptr)
        {
            return false;
        }
        mString = mLibrary->getSchema("String");
        mInt = mLibrary->getSchema("Int");
        mFloat = mLibrary->getSchema("Float");
        mBool = mLibrary->getSchema("Bool");
        if (mString == nullptr || mInt == nullptr || mFloat == nullptr || mBool == nullptr)
        {
            return false;
        }
        if (install)
        {
            Schema* instanceNames = mLibrary->getSchema("InstanceNames");
            if (instanceNames == nullptr)
            {
                instanceNames = mLibrary->addSchema(rose::SchemaType::List, "InstanceNames");
                if (instanceNames == nullptr)
                {
                    return false;
                }
                instanceNames->setElementSchema(mString);
            }
        }
        return true;
    }

    void TableDocument::clearActive()
    {
        mActiveSchema = -1;
        mActiveInstance = -1;
    }

    bool TableDocument::isScaffoldingSchemaName(const String& name) const
    {
        if (name == "None" || name == "InstanceNames")
        {
            return true;
        }
        return name.startsWith("Row_") || name.startsWith("Rows_");
    }

    bool TableDocument::isReservedSchemaName(const String& name) const
    {
        return isBuildinSchema(name) || this->isScaffoldingSchemaName(name);
    }

    bool TableDocument::isUserSchemaName(const String& name) const
    {
        return !name.isEmpty() && !this->isReservedSchemaName(name);
    }

    rose::Schema* TableDocument::userSchemaAt(u32_t index) const
    {
        if (mLibrary == nullptr)
        {
            return nullptr;
        }
        u32_t seen = 0;
        u32_t count = mLibrary->getSchemaCount();
        for (u32_t i = 0; i < count; ++i)
        {
            Schema* schema = mLibrary->getSchema(i);
            if (schema == nullptr || !this->isUserSchemaName(schema->name()))
            {
                continue;
            }
            if (seen == index)
            {
                return schema;
            }
            ++seen;
        }
        return nullptr;
    }

    rose::Schema* TableDocument::activeSchema() const
    {
        if (!this->hasActiveSchema())
        {
            return nullptr;
        }
        return this->userSchemaAt(this->activeSchemaIndex());
    }

    u32_t TableDocument::userSchemaCount() const
    {
        if (mLibrary == nullptr)
        {
            return 0;
        }
        u32_t seen = 0;
        u32_t count = mLibrary->getSchemaCount();
        for (u32_t i = 0; i < count; ++i)
        {
            Schema* schema = mLibrary->getSchema(i);
            if (schema != nullptr && this->isUserSchemaName(schema->name()))
            {
                ++seen;
            }
        }
        return seen;
    }

    bool TableDocument::hasActiveSchema() const
    {
        return mOpen && mLibrary != nullptr && mActiveSchema >= 0
            && static_cast<u32_t>(mActiveSchema) < this->userSchemaCount()
            && this->userSchemaAt(static_cast<u32_t>(mActiveSchema)) != nullptr;
    }

    bool TableDocument::selectSchema(u32_t index)
    {
        if (!mOpen || index >= this->userSchemaCount())
        {
            return false;
        }
        mActiveSchema = static_cast<int>(index);
        return true;
    }

    String TableDocument::schemaName(u32_t index) const
    {
        Schema* schema = this->userSchemaAt(index);
        if (schema == nullptr)
        {
            return String::empty;
        }
        return schema->name();
    }

    rose::SchemaType TableDocument::schemaType(u32_t index) const
    {
        Schema* schema = this->userSchemaAt(index);
        if (schema == nullptr)
        {
            return rose::SchemaType::None;
        }
        return schema->type();
    }

    String TableDocument::nextNewSchemaName() const
    {
        for (u32_t number = 1; ; ++number)
        {
            String name = String::format("Schema%u", number);
            if (!this->schemaNameTaken(name, -1) && mLibrary->getSchema(name) == nullptr)
            {
                return name;
            }
        }
    }

    bool TableDocument::schemaNameTaken(const String& name, int skipIndex) const
    {
        u32_t count = this->userSchemaCount();
        for (u32_t index = 0; index < count; ++index)
        {
            if (skipIndex >= 0 && static_cast<int>(index) == skipIndex)
            {
                continue;
            }
            if (this->schemaName(index) == name)
            {
                return true;
            }
        }
        return false;
    }

    bool TableDocument::schemaReaches(rose::Schema* start, rose::Schema* target) const
    {
        if (start == nullptr || target == nullptr)
        {
            return false;
        }
        std::set<Schema*> seen;
        std::function<bool(Schema*)> walk = [&](Schema* node) -> bool
        {
            if (node == nullptr || seen.find(node) != seen.end())
            {
                return false;
            }
            if (node == target)
            {
                return true;
            }
            seen.insert(node);
            if (node->is(rose::SchemaType::List))
            {
                return walk(node->getElementSchema());
            }
            if (node->is(rose::SchemaType::Struct))
            {
                u32_t members = node->getMemberCount();
                for (u32_t i = 0; i < members; ++i)
                {
                    Schema::Member* member = node->getMember(i);
                    if (member != nullptr && walk(member->schema))
                    {
                        return true;
                    }
                }
            }
            return false;
        };
        return walk(start);
    }

    bool TableDocument::wouldCycle(rose::Schema* from, rose::Schema* to) const
    {
        if (from == nullptr || to == nullptr)
        {
            return false;
        }
        return this->schemaReaches(to, from);
    }

    bool TableDocument::schemaReferenced(rose::Schema* schema) const
    {
        if (schema == nullptr || mLibrary == nullptr)
        {
            return false;
        }
        if (mLibrary->hasValueDependOnSchema(schema))
        {
            return true;
        }
        u32_t count = mLibrary->getSchemaCount();
        for (u32_t i = 0; i < count; ++i)
        {
            Schema* other = mLibrary->getSchema(i);
            if (other == nullptr || other == schema)
            {
                continue;
            }
            if (other->is(rose::SchemaType::List) && other->getElementSchema() == schema)
            {
                return true;
            }
            if (other->is(rose::SchemaType::Struct))
            {
                u32_t members = other->getMemberCount();
                for (u32_t m = 0; m < members; ++m)
                {
                    Schema::Member* member = other->getMember(m);
                    if (member != nullptr && member->schema == schema)
                    {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    String TableDocument::nextMemberName(rose::Schema* schema) const
    {
        for (u32_t number = 1; ; ++number)
        {
            String name = String::format("field%u", number);
            if (schema == nullptr || schema->getMember(name) == nullptr)
            {
                return name;
            }
        }
    }

    rose::Schema* TableDocument::resolveAssignableType(const String& typeName, rose::Schema* current) const
    {
        if (mLibrary == nullptr || typeName.isEmpty())
        {
            return nullptr;
        }
        Schema* next = mLibrary->getSchema(typeName);
        if (next == nullptr)
        {
            return nullptr;
        }
        if (next == current || this->wouldCycle(current, next))
        {
            return nullptr;
        }
        if (isBuildinSchema(typeName) || this->isUserSchemaName(typeName))
        {
            return next;
        }
        return nullptr;
    }

    bool TableDocument::addUserSchema(rose::SchemaType type)
    {
        if (!mOpen || mLibrary == nullptr)
        {
            this->setStatus(DocStatus::CannotCreateSchema);
            return false;
        }
        if (type != rose::SchemaType::Struct && type != rose::SchemaType::List
            && type != rose::SchemaType::Options)
        {
            this->setStatus(DocStatus::CannotCreateSchema);
            return false;
        }
        String name = this->nextNewSchemaName();
        Schema* schema = mLibrary->addSchema(type, name);
        if (schema == nullptr)
        {
            this->setStatus(DocStatus::CannotCreateSchema);
            return false;
        }
        if (type == rose::SchemaType::List)
        {
            schema->setElementSchema(mString);
        }
        this->markDirty();
        this->setStatus(DocStatus::Ready);
        u32_t count = this->userSchemaCount();
        if (count > 0)
        {
            mActiveSchema = static_cast<int>(count - 1);
        }
        return true;
    }

    bool TableDocument::renameUserSchema(u32_t index, const String& name)
    {
        if (!mOpen || mLibrary == nullptr || index >= this->userSchemaCount())
        {
            return false;
        }
        Schema* schema = this->userSchemaAt(index);
        if (schema == nullptr)
        {
            return false;
        }
        String next = name.trim();
        if (next.isEmpty())
        {
            this->setStatus(DocStatus::SchemaNameEmpty);
            return false;
        }
        if (this->isReservedSchemaName(next))
        {
            this->setStatus(DocStatus::SchemaNameTaken);
            return false;
        }
        if (schema->name() == next)
        {
            return true;
        }
        if (mLibrary->getSchema(next) != nullptr)
        {
            this->setStatus(DocStatus::SchemaNameTaken);
            return false;
        }
        String from = schema->name();
        if (!mLibrary->renameSchema(from, next))
        {
            this->setStatus(DocStatus::SchemaNameTaken);
            return false;
        }
        this->markDirty();
        this->setStatus(DocStatus::Ready);
        return true;
    }

    bool TableDocument::deleteUserSchema(u32_t index)
    {
        if (!mOpen || mLibrary == nullptr || index >= this->userSchemaCount())
        {
            return false;
        }
        Schema* schema = this->userSchemaAt(index);
        if (schema == nullptr)
        {
            return false;
        }
        if (this->isReservedSchemaName(schema->name()) || this->schemaReferenced(schema))
        {
            this->setStatus(DocStatus::CannotDeleteSchema);
            return false;
        }
        String name = schema->name();
        if (!mLibrary->removeSchema(name))
        {
            this->setStatus(DocStatus::CannotDeleteSchema);
            return false;
        }
        this->markDirty();
        this->setStatus(DocStatus::Ready);
        u32_t count = this->userSchemaCount();
        if (mActiveSchema == static_cast<int>(index))
        {
            if (count == 0)
            {
                mActiveSchema = -1;
            }
            else if (static_cast<u32_t>(mActiveSchema) >= count)
            {
                mActiveSchema = static_cast<int>(count - 1);
            }
        }
        else if (mActiveSchema > static_cast<int>(index))
        {
            --mActiveSchema;
        }
        return true;
    }

    u32_t TableDocument::schemaMemberCount() const
    {
        Schema* schema = this->activeSchema();
        if (schema == nullptr || !schema->is(rose::SchemaType::Struct))
        {
            return 0;
        }
        return schema->getMemberCount();
    }

    String TableDocument::schemaMemberName(u32_t memberIndex) const
    {
        Schema* schema = this->activeSchema();
        if (schema == nullptr || memberIndex >= schema->getMemberCount())
        {
            return String::empty;
        }
        Schema::Member* member = schema->getMember(memberIndex);
        if (member == nullptr)
        {
            return String::empty;
        }
        return member->name;
    }

    String TableDocument::schemaMemberTypeName(u32_t memberIndex) const
    {
        Schema* schema = this->activeSchema();
        if (schema == nullptr || memberIndex >= schema->getMemberCount())
        {
            return String::empty;
        }
        Schema::Member* member = schema->getMember(memberIndex);
        if (member == nullptr || member->schema == nullptr)
        {
            return String::empty;
        }
        return member->schema->name();
    }

    bool TableDocument::addSchemaMember()
    {
        Schema* schema = this->activeSchema();
        if (schema == nullptr || !schema->is(rose::SchemaType::Struct) || mString == nullptr)
        {
            return false;
        }
        if (!schema->addMember(this->nextMemberName(schema), mString))
        {
            this->setStatus(DocStatus::CannotAddMember);
            return false;
        }
        this->markDirty();
        this->setStatus(DocStatus::Ready);
        return true;
    }

    bool TableDocument::renameSchemaMember(u32_t memberIndex, const String& name)
    {
        Schema* schema = this->activeSchema();
        if (schema == nullptr || !schema->is(rose::SchemaType::Struct) || memberIndex >= schema->getMemberCount())
        {
            return false;
        }
        Schema::Member* member = schema->getMember(memberIndex);
        if (member == nullptr)
        {
            return false;
        }
        String next = name.trim();
        if (next.isEmpty())
        {
            this->setStatus(DocStatus::MemberNameEmpty);
            return false;
        }
        if (member->name == next)
        {
            return true;
        }
        if (schema->getMember(next) != nullptr)
        {
            this->setStatus(DocStatus::MemberNameTaken);
            return false;
        }
        member->name = next;
        this->markDirty();
        this->setStatus(DocStatus::Ready);
        return true;
    }

    bool TableDocument::setSchemaMemberType(u32_t memberIndex, const String& typeName)
    {
        Schema* schema = this->activeSchema();
        if (schema == nullptr || !schema->is(rose::SchemaType::Struct) || memberIndex >= schema->getMemberCount())
        {
            return false;
        }
        Schema::Member* member = schema->getMember(memberIndex);
        if (member == nullptr)
        {
            return false;
        }
        Schema* next = this->resolveAssignableType(typeName, schema);
        if (next == nullptr)
        {
            return false;
        }
        if (!schema->setMember(member->name, next))
        {
            return false;
        }
        this->markDirty();
        this->setStatus(DocStatus::Ready);
        return true;
    }

    bool TableDocument::moveSchemaMember(u32_t memberIndex, int delta)
    {
        Schema* schema = this->activeSchema();
        if (schema == nullptr || !schema->is(rose::SchemaType::Struct) || delta == 0)
        {
            return false;
        }
        u32_t count = schema->getMemberCount();
        if (memberIndex >= count)
        {
            return false;
        }
        int next = static_cast<int>(memberIndex) + delta;
        if (next < 0 || static_cast<u32_t>(next) >= count)
        {
            return false;
        }
        Schema::Member* a = schema->getMember(memberIndex);
        Schema::Member* b = schema->getMember(static_cast<u32_t>(next));
        if (a == nullptr || b == nullptr)
        {
            return false;
        }
        std::swap(*a, *b);
        this->markDirty();
        this->setStatus(DocStatus::Ready);
        return true;
    }

    bool TableDocument::deleteSchemaMember(u32_t memberIndex)
    {
        Schema* schema = this->activeSchema();
        if (schema == nullptr || !schema->is(rose::SchemaType::Struct) || memberIndex >= schema->getMemberCount())
        {
            return false;
        }
        Schema::Member* member = schema->getMember(memberIndex);
        if (member == nullptr)
        {
            return false;
        }
        schema->delMember(member->name);
        this->markDirty();
        this->setStatus(DocStatus::Ready);
        return true;
    }

    String TableDocument::schemaElementTypeName() const
    {
        Schema* schema = this->activeSchema();
        if (schema == nullptr || !schema->is(rose::SchemaType::List))
        {
            return String::empty;
        }
        Schema* element = schema->getElementSchema();
        if (element == nullptr)
        {
            return String::empty;
        }
        return element->name();
    }

    bool TableDocument::setSchemaElementType(const String& typeName)
    {
        Schema* schema = this->activeSchema();
        if (schema == nullptr || !schema->is(rose::SchemaType::List))
        {
            return false;
        }
        Schema* next = this->resolveAssignableType(typeName, schema);
        if (next == nullptr)
        {
            return false;
        }
        schema->setElementSchema(next);
        this->markDirty();
        this->setStatus(DocStatus::Ready);
        return true;
    }

    std::vector<String> TableDocument::schemaAssignableTypeNames() const
    {
        std::vector<String> names;
        if (mLibrary == nullptr)
        {
            return names;
        }
        Schema* current = this->activeSchema();
        for (u32_t i = 0; i < buildinSchemaCount(); ++i)
        {
            const char* builtin = buildinSchemaName(i);
            Schema* candidate = mLibrary->getSchema(builtin);
            if (candidate == nullptr || this->wouldCycle(current, candidate))
            {
                continue;
            }
            names.push_back(String(builtin));
        }
        u32_t count = this->userSchemaCount();
        for (u32_t i = 0; i < count; ++i)
        {
            Schema* candidate = this->userSchemaAt(i);
            if (candidate == nullptr || candidate == current || this->wouldCycle(current, candidate))
            {
                continue;
            }
            names.push_back(candidate->name());
        }
        return names;
    }

    std::vector<String> TableDocument::columnTypeNames() const
    {
        std::vector<String> names;
        for (u32_t i = 0; i < buildinSchemaCount(); ++i)
        {
            names.push_back(String(buildinSchemaName(i)));
        }
        u32_t count = this->userSchemaCount();
        for (u32_t i = 0; i < count; ++i)
        {
            Schema* schema = this->userSchemaAt(i);
            if (schema != nullptr)
            {
                names.push_back(schema->name());
            }
        }
        return names;
    }

    bool TableDocument::isListType(const String& typeName) const
    {
        if (!this->isNestedColumnType(typeName))
        {
            return false;
        }
        Schema* schema = this->storageForType(typeName);
        return schema != nullptr && schema->is(rose::SchemaType::List);
    }

    NestedField TableDocument::fieldFromStorage(rose::Schema* storage, const String& id, const String& name) const
    {
        NestedField field;
        field.id = id;
        field.name = name;
        field.schemaName = storage != nullptr ? storage->name() : String("String");
        field.kind = this->fieldKindForColumnType(field.schemaName);
        field.options = fieldOptionsFromSchema(optionsSchemaOf(storage));
        field.nestedStruct = storage != nullptr && storage->is(rose::SchemaType::Struct);
        field.nestedList = this->isListType(field.schemaName);
        return field;
    }

    String TableDocument::displayFromValue(rose::Value* field, const NestedField& info) const
    {
        if (field == nullptr || field->schema == nullptr || field->schema->is(rose::SchemaType::None))
        {
            return String::empty;
        }
        if (info.nestedStruct)
        {
            return String::format("{%u}", field->schema->getMemberCount());
        }
        if (info.nestedList)
        {
            return String::format("[%u]", mLibrary->count(field));
        }
        if (info.kind == FieldKind::MultiOption)
        {
            return joinSelected(this->labelsFromValue(field, info));
        }
        if (field->schema->is(rose::SchemaType::Int))
        {
            i64_t value = 0;
            return mLibrary->get(field, value) ? String::format("%lld", static_cast<long long>(value)) : String::empty;
        }
        if (field->schema->is(rose::SchemaType::Float))
        {
            f64_t value = 0;
            return mLibrary->get(field, value) ? String::format("%.15g", value) : String::empty;
        }
        if (field->schema->is(rose::SchemaType::Bool))
        {
            bool value = false;
            if (!mLibrary->get(field, value))
            {
                return String::empty;
            }
            return value ? "true" : "false";
        }
        String value;
        if (!mLibrary->get(field, value))
        {
            return String::empty;
        }
        if (info.kind == FieldKind::Date)
        {
            return displayDate(value);
        }
        if (info.kind == FieldKind::Time)
        {
            return displayTime(value);
        }
        if (info.kind == FieldKind::DateTime)
        {
            return displayDateTime(value);
        }
        if (info.kind == FieldKind::Color)
        {
            Color color;
            if (parseColor(value, color))
            {
                return formatColor(color);
            }
        }
        if (info.kind == FieldKind::SingleOption)
        {
            return optionDisplayLabel(info.options, value);
        }
        return value;
    }

    String TableDocument::textFromValue(rose::Value* field, const NestedField& info) const
    {
        return this->displayFromValue(field, info);
    }

    std::vector<String> TableDocument::labelsFromValue(rose::Value* field, const NestedField& info) const
    {
        std::vector<String> names;
        if (field == nullptr || field->schema == nullptr || field->schema->is(rose::SchemaType::None))
        {
            return {};
        }
        if (field->schema->is(rose::SchemaType::List))
        {
            u32_t count = mLibrary->count(field);
            for (u32_t index = 0; index < count; ++index)
            {
                String item;
                if (mLibrary->get(field, index, item) && !item.isEmpty())
                {
                    names.push_back(item);
                }
            }
        }
        else
        {
            String value;
            if (mLibrary->get(field, value) && !value.isEmpty())
            {
                names.push_back(value);
            }
        }
        std::vector<String> labels;
        for (const String& name : names)
        {
            labels.push_back(optionDisplayLabel(info.options, name));
        }
        return this->filterLabels(labels, info.options);
    }

    rose::Value* TableDocument::makeStoredValue(FieldKind kind, const String& schemaName, const std::vector<FieldOption>& options, const String& text, bool nested)
    {
        if (mLibrary == nullptr)
        {
            return nullptr;
        }
        if (nested)
        {
            Schema* schema = this->storageForType(schemaName);
            if (trimCopy(text).isEmpty())
            {
                return this->makeNone();
            }
            return schema != nullptr ? mLibrary->make(schema) : nullptr;
        }
        String value = trimCopy(text);
        if (kind == FieldKind::SingleOption)
        {
            String name = optionStorageNameFromLabel(options, value);
            if (!name.isEmpty())
            {
                value = name;
            }
        }
        Schema* schema = this->storageForType(schemaName);
        if (value.isEmpty() || !validateField(kind, value))
        {
            return this->makeNone();
        }
        Value* made = mLibrary->make(schema);
        if (made == nullptr)
        {
            return nullptr;
        }
        switch (kind)
        {
        case FieldKind::Integer:
        {
            i64_t number = 0;
            parseInteger(value, number);
            mLibrary->set(made, number);
            break;
        }
        case FieldKind::Float:
        {
            f64_t number = 0;
            parseFloat(value, number);
            mLibrary->set(made, number);
            break;
        }
        case FieldKind::Boolean:
        {
            bool flag = false;
            parseBool(value, flag);
            mLibrary->set(made, flag);
            break;
        }
        default:
            mLibrary->set(made, canonicalText(kind, value));
            break;
        }
        return made;
    }

    bool TableDocument::writeFieldValue(rose::Value* parent, const NestedHop& hop, const NestedField& info, const String& text)
    {
        if (parent == nullptr || mLibrary == nullptr)
        {
            return false;
        }
        if (info.kind == FieldKind::MultiOption)
        {
            this->writeFieldLabels(parent, hop, info, splitSelected(text));
            return true;
        }
        String trimmed = trimCopy(text);
        if (!hop.list && parent->schema != nullptr && parent->schema->is(rose::SchemaType::Struct))
        {
            Value* slot = this->fieldAt(parent, hop.member);
            if (slot != nullptr && slot->schema != nullptr && slot->schema->is(rose::SchemaType::None))
            {
                Value* made = this->makeStoredValue(info.kind, info.schemaName, info.options, text, false);
                if (made == nullptr)
                {
                    return false;
                }
                return mLibrary->set(parent, hop.member, made);
            }
            if (slot != nullptr && slot->schema != nullptr && !slot->schema->is(rose::SchemaType::None))
            {
                if (trimmed.isEmpty())
                {
                    Value* none = this->makeNone();
                    if (none == nullptr)
                    {
                        return false;
                    }
                    return mLibrary->set(parent, hop.member, none);
                }
                if (slot->schema->is(rose::SchemaType::String))
                {
                    return mLibrary->set(slot, canonicalText(info.kind, trimmed));
                }
                if (slot->schema->is(rose::SchemaType::Int))
                {
                    i64_t number = 0;
                    if (!parseInteger(trimmed, number))
                    {
                        return false;
                    }
                    return mLibrary->set(slot, number);
                }
                if (slot->schema->is(rose::SchemaType::Float))
                {
                    f64_t number = 0;
                    if (!parseFloat(trimmed, number))
                    {
                        return false;
                    }
                    return mLibrary->set(slot, number);
                }
                if (slot->schema->is(rose::SchemaType::Bool))
                {
                    bool flag = false;
                    if (!parseBool(trimmed, flag))
                    {
                        return false;
                    }
                    return mLibrary->set(slot, flag);
                }
            }
        }
        Value* made = this->makeStoredValue(info.kind, info.schemaName, info.options, text, info.nestedStruct || info.nestedList);
        if (made == nullptr)
        {
            return false;
        }
        if (hop.list)
        {
            return mLibrary->set(parent, hop.index, made);
        }
        return mLibrary->set(parent, hop.member, made);
    }

    void TableDocument::writeFieldLabels(rose::Value* parent, const NestedHop& hop, const NestedField& info, const std::vector<String>& labels)
    {
        if (parent == nullptr || mLibrary == nullptr)
        {
            return;
        }
        std::vector<String> filtered = this->filterLabels(labels, info.options);
        Value* made = nullptr;
        if (filtered.empty())
        {
            made = this->makeNone();
        }
        else
        {
            Schema* storage = this->storageForType(info.schemaName);
            made = mLibrary->make(storage);
            if (made != nullptr)
            {
                Schema* opts = optionsSchemaOf(storage);
                mLibrary->clear(made);
                for (const String& label : filtered)
                {
                    String name = optionStorageNameFromLabel(info.options, label);
                    if (name.isEmpty())
                    {
                        continue;
                    }
                    if (info.kind == FieldKind::SingleOption)
                    {
                        mLibrary->set(made, name);
                        break;
                    }
                    Value* item = opts != nullptr ? mLibrary->make(opts) : mLibrary->make(mString);
                    if (item != nullptr)
                    {
                        mLibrary->set(item, name);
                        mLibrary->push(made, item);
                    }
                }
            }
        }
        if (made == nullptr)
        {
            return;
        }
        if (hop.list)
        {
            mLibrary->set(parent, hop.index, made);
        }
        else
        {
            mLibrary->set(parent, hop.member, made);
        }
    }

    std::vector<FieldOption> TableDocument::schemaOptions() const
    {
        return fieldOptionsFromSchema(this->activeSchema());
    }

    bool TableDocument::setSchemaOptions(const std::vector<FieldOption>& options)
    {
        Schema* schema = this->activeSchema();
        if (schema == nullptr || !schema->is(rose::SchemaType::Options))
        {
            return false;
        }
        if (optionNameConflict(options))
        {
            this->setStatus(DocStatus::OptionNameTaken);
            return false;
        }
        if (!replaceSchemaOptions(schema, options))
        {
            this->setStatus(DocStatus::CannotUpdateOptions);
            return false;
        }
        this->markDirty();
        this->setStatus(DocStatus::Ready);
        return true;
    }

    FieldKind TableDocument::fieldKindForColumnType(const String& typeName) const
    {
        String name = canonicalizeColumnType(typeName);
        if (mLibrary != nullptr)
        {
            Schema* schema = mLibrary->getSchema(name);
            if (schema != nullptr)
            {
                if (isOptionsSchema(schema))
                {
                    return FieldKind::SingleOption;
                }
                if (isMultiOptionsList(schema))
                {
                    return FieldKind::MultiOption;
                }
            }
        }
        return fieldKindFromSchemaName(name);
    }

    rose::Schema* TableDocument::storageForType(const String& typeName) const
    {
        String name = canonicalizeColumnType(typeName);
        if (mLibrary == nullptr)
        {
            return mString;
        }
        Schema* schema = mLibrary->getSchema(name);
        return schema != nullptr ? schema : mString;
    }

    bool TableDocument::isNestedColumnType(const String& typeName) const
    {
        String name = canonicalizeColumnType(typeName);
        if (isBuildinSchema(name))
        {
            return false;
        }
        Schema* schema = mLibrary != nullptr ? mLibrary->getSchema(name) : nullptr;
        if (schema == nullptr)
        {
            return false;
        }
        if (isOptionsSchema(schema) || isMultiOptionsList(schema))
        {
            return false;
        }
        return schema->is(rose::SchemaType::Struct) || schema->is(rose::SchemaType::List);
    }

    rose::Schema* TableDocument::noneSchema()
    {
        rose::Schema* schema = mLibrary->getSchema("None");
        if (schema == nullptr)
        {
            schema = mLibrary->addSchema(rose::SchemaType::None, "None");
        }
        return schema;
    }

    rose::Value* TableDocument::makeNone()
    {
        rose::Schema* none = this->noneSchema();
        Value* made = mLibrary->make(mString);
        if (made == nullptr || none == nullptr)
        {
            return nullptr;
        }
        made->schema = none;
        return made;
    }

    rose::Value* TableDocument::fieldAt(rose::Value* object, const String& field) const
    {
        if (mLibrary == nullptr || object == nullptr || object->schema == nullptr)
        {
            return nullptr;
        }
        if (!object->schema->is(rose::SchemaType::Struct))
        {
            return nullptr;
        }
        try
        {
            return mLibrary->get(object, field);
        }
        catch (const std::out_of_range&)
        {
            return nullptr;
        }
    }

    std::vector<String> TableDocument::filterLabels(const std::vector<String>& labels, const std::vector<FieldOption>& options) const
    {
        std::vector<String> out;
        for (const FieldOption& option : options)
        {
            if (option.label.isEmpty())
            {
                continue;
            }
            if (selectedContains(labels, option.label) && !selectedContains(out, option.label))
            {
                out.push_back(option.label);
            }
        }
        return out;
    }

    bool TableDocument::ensureInstancesRegistry()
    {
        if (mLibrary == nullptr || mString == nullptr)
        {
            return false;
        }
        mInstances = mLibrary->get("instances");
        if (mInstances != nullptr)
        {
            return true;
        }
        Schema* listSchema = mLibrary->getSchema("InstanceNames");
        if (listSchema == nullptr)
        {
            listSchema = mLibrary->addSchema(rose::SchemaType::List, "InstanceNames");
            if (listSchema == nullptr)
            {
                return false;
            }
            listSchema->setElementSchema(mString);
        }
        mInstances = mLibrary->make(listSchema);
        if (mInstances == nullptr)
        {
            return false;
        }
        mLibrary->set("instances", mInstances);
        return true;
    }

    bool TableDocument::isReservedRootName(const String& name) const
    {
        return name == "tables" || name == "instances";
    }

    String TableDocument::instanceKeyAt(u32_t index) const
    {
        if (mLibrary == nullptr || mInstances == nullptr || index >= mLibrary->count(mInstances))
        {
            return String::empty;
        }
        String key;
        if (!mLibrary->get(mInstances, index, key))
        {
            return String::empty;
        }
        return key;
    }

    u32_t TableDocument::instanceCount() const
    {
        if (mLibrary == nullptr || mInstances == nullptr)
        {
            return 0;
        }
        return mLibrary->count(mInstances);
    }

    bool TableDocument::hasActiveInstance() const
    {
        return mOpen && mLibrary != nullptr && mActiveInstance >= 0
            && static_cast<u32_t>(mActiveInstance) < this->instanceCount();
    }

    u32_t TableDocument::activeInstanceIndex() const
    {
        return mActiveInstance < 0 ? 0 : static_cast<u32_t>(mActiveInstance);
    }

    bool TableDocument::selectInstance(u32_t index)
    {
        if (!mOpen || index >= this->instanceCount())
        {
            return false;
        }
        mActiveInstance = static_cast<int>(index);
        return true;
    }

    String TableDocument::instanceName(u32_t index) const
    {
        return this->instanceKeyAt(index);
    }

    rose::Value* TableDocument::instanceValue(u32_t index) const
    {
        if (mLibrary == nullptr)
        {
            return nullptr;
        }
        String key = this->instanceKeyAt(index);
        if (key.isEmpty())
        {
            return nullptr;
        }
        return mLibrary->get(key);
    }

    rose::Value* TableDocument::activeInstanceValue() const
    {
        if (!this->hasActiveInstance())
        {
            return nullptr;
        }
        return this->instanceValue(static_cast<u32_t>(mActiveInstance));
    }

    String TableDocument::instanceSchemaName(u32_t index) const
    {
        Value* value = this->instanceValue(index);
        if (value == nullptr || value->schema == nullptr)
        {
            return String::empty;
        }
        return value->schema->name();
    }

    bool TableDocument::instanceNameTaken(const String& name, int skipIndex) const
    {
        String next = name.trim();
        if (next.isEmpty() || this->isReservedRootName(next))
        {
            return true;
        }
        u32_t count = this->instanceCount();
        for (u32_t i = 0; i < count; ++i)
        {
            if (skipIndex >= 0 && static_cast<int>(i) == skipIndex)
            {
                continue;
            }
            if (this->instanceKeyAt(i) == next)
            {
                return true;
            }
        }
        Value* self = skipIndex >= 0 ? this->instanceValue(static_cast<u32_t>(skipIndex)) : nullptr;
        Value* existing = mLibrary != nullptr ? mLibrary->get(next) : nullptr;
        return existing != nullptr && existing != self;
    }

    String TableDocument::nextNewInstanceName() const
    {
        for (u32_t number = 1; number < 100000; ++number)
        {
            String name = String::format("Instance %u", number);
            if (!this->instanceNameTaken(name, -1))
            {
                return name;
            }
        }
        return String("Instance");
    }

    bool TableDocument::addInstance(const String& schemaName)
    {
        if (!mOpen || mLibrary == nullptr || !this->ensureInstancesRegistry())
        {
            return false;
        }
        Schema* schema = mLibrary->getSchema(schemaName);
        if (schema == nullptr)
        {
            this->setStatus(DocStatus::CannotCreateInstance);
            return false;
        }
        Value* made = mLibrary->make(schema);
        if (made == nullptr)
        {
            this->setStatus(DocStatus::CannotCreateInstance);
            return false;
        }
        String name = this->nextNewInstanceName();
        mLibrary->set(name, made);
        if (!mLibrary->push(mInstances, name))
        {
            mLibrary->removeRoot(name);
            this->setStatus(DocStatus::CannotCreateInstance);
            return false;
        }
        mActiveInstance = static_cast<int>(this->instanceCount() - 1);
        this->markDirty();
        this->setStatus(DocStatus::Ready);
        return true;
    }

    bool TableDocument::renameInstance(u32_t index, const String& name)
    {
        if (!mOpen || mLibrary == nullptr || index >= this->instanceCount())
        {
            return false;
        }
        String next = name.trim();
        if (next.isEmpty())
        {
            this->setStatus(DocStatus::InstanceNameEmpty);
            return false;
        }
        if (this->instanceNameTaken(next, static_cast<int>(index)))
        {
            this->setStatus(DocStatus::InstanceNameTaken);
            return false;
        }
        String from = this->instanceKeyAt(index);
        if (from.isEmpty() || from == next)
        {
            this->setStatus(DocStatus::Ready);
            return true;
        }
        if (!mLibrary->renameRoot(from, next))
        {
            this->setStatus(DocStatus::InstanceNameTaken);
            return false;
        }
        if (!mLibrary->set(mInstances, index, next))
        {
            mLibrary->renameRoot(next, from);
            this->setStatus(DocStatus::InstanceNameTaken);
            return false;
        }
        this->markDirty();
        this->setStatus(DocStatus::Ready);
        return true;
    }

    bool TableDocument::deleteInstance(u32_t index)
    {
        if (!mOpen || mLibrary == nullptr || index >= this->instanceCount())
        {
            return false;
        }
        String key = this->instanceKeyAt(index);
        mLibrary->remove(mInstances, index);
        mLibrary->removeRoot(key);
        u32_t count = this->instanceCount();
        if (count == 0)
        {
            mActiveInstance = -1;
        }
        else if (mActiveInstance == static_cast<int>(index))
        {
            if (static_cast<u32_t>(index) >= count)
            {
                mActiveInstance = static_cast<int>(count - 1);
            }
        }
        else if (mActiveInstance > static_cast<int>(index))
        {
            --mActiveInstance;
        }
        this->markDirty();
        this->setStatus(DocStatus::Ready);
        return true;
    }

    NestedField TableDocument::instanceRootField() const
    {
        Value* value = this->activeInstanceValue();
        if (value == nullptr || value->schema == nullptr)
        {
            return NestedField();
        }
        return this->fieldFromStorage(value->schema, String::empty, String::empty);
    }

    String TableDocument::instanceText() const
    {
        Value* value = this->activeInstanceValue();
        if (value == nullptr)
        {
            return String::empty;
        }
        NestedField info = this->instanceRootField();
        return this->textFromValue(value, info);
    }

    bool TableDocument::setInstanceText(const String& text)
    {
        Value* root = this->activeInstanceValue();
        if (!mOpen || root == nullptr || mLibrary == nullptr || root->schema == nullptr)
        {
            return false;
        }
        NestedField info = this->instanceRootField();
        if (info.nestedStruct || info.nestedList)
        {
            return false;
        }
        if (info.kind == FieldKind::MultiOption)
        {
            return this->setInstanceLabels(splitSelected(text));
        }
        if (info.kind == FieldKind::SingleOption)
        {
            String name = optionStorageNameFromLabel(info.options, trimCopy(text));
            String before;
            mLibrary->get(root, before);
            if (name.isEmpty())
            {
                mLibrary->set(root, String::empty);
            }
            else
            {
                mLibrary->set(root, name);
            }
            String after;
            mLibrary->get(root, after);
            if (before != after)
            {
                this->markDirty();
            }
            return true;
        }
        String trimmed = trimCopy(text);
        if (!trimmed.isEmpty() && !validateField(info.kind, trimmed))
        {
            this->setStatus(DocStatus::TypeMismatchCleared);
        }
        String before = this->instanceText();
        if (trimmed.isEmpty())
        {
            if (root->schema->is(rose::SchemaType::Int))
            {
                mLibrary->set(root, static_cast<i64_t>(0));
            }
            else if (root->schema->is(rose::SchemaType::Float))
            {
                mLibrary->set(root, static_cast<f64_t>(0));
            }
            else if (root->schema->is(rose::SchemaType::Bool))
            {
                mLibrary->set(root, false);
            }
            else
            {
                mLibrary->set(root, String::empty);
            }
        }
        else if (info.kind == FieldKind::Integer)
        {
            i64_t number = 0;
            parseInteger(trimmed, number);
            mLibrary->set(root, number);
        }
        else if (info.kind == FieldKind::Float)
        {
            f64_t number = 0;
            parseFloat(trimmed, number);
            mLibrary->set(root, number);
        }
        else if (info.kind == FieldKind::Boolean)
        {
            bool flag = false;
            parseBool(trimmed, flag);
            mLibrary->set(root, flag);
        }
        else
        {
            mLibrary->set(root, canonicalText(info.kind, trimmed));
        }
        if (before != this->instanceText())
        {
            this->markDirty();
        }
        return true;
    }

    std::vector<String> TableDocument::instanceLabels() const
    {
        Value* value = this->activeInstanceValue();
        if (value == nullptr)
        {
            return {};
        }
        return this->labelsFromValue(value, this->instanceRootField());
    }

    bool TableDocument::setInstanceLabels(const std::vector<String>& labels)
    {
        Value* root = this->activeInstanceValue();
        if (!mOpen || root == nullptr || mLibrary == nullptr)
        {
            return false;
        }
        NestedField info = this->instanceRootField();
        std::vector<String> filtered = this->filterLabels(labels, info.options);
        if (info.kind == FieldKind::SingleOption)
        {
            String name = filtered.empty() ? String::empty : optionStorageNameFromLabel(info.options, filtered[0]);
            if (name.isEmpty())
            {
                mLibrary->set(root, String::empty);
            }
            else
            {
                mLibrary->set(root, name);
            }
            this->markDirty();
            return true;
        }
        if (info.kind != FieldKind::MultiOption || root->schema == nullptr || !root->schema->is(rose::SchemaType::List))
        {
            return false;
        }
        Schema* opts = optionsSchemaOf(root->schema);
        mLibrary->clear(root);
        for (const String& label : filtered)
        {
            String name = optionStorageNameFromLabel(info.options, label);
            if (name.isEmpty())
            {
                continue;
            }
            Value* item = opts != nullptr ? mLibrary->make(opts) : mLibrary->make(mString);
            if (item == nullptr)
            {
                continue;
            }
            mLibrary->set(item, name);
            mLibrary->push(root, item);
        }
        this->markDirty();
        return true;
    }

    rose::Schema* TableDocument::instanceSchemaAtPath(const std::vector<NestedHop>& path) const
    {
        Value* value = this->activeInstanceValue();
        if (value == nullptr || value->schema == nullptr)
        {
            return nullptr;
        }
        Schema* schema = value->schema;
        for (const NestedHop& hop : path)
        {
            if (schema == nullptr)
            {
                return nullptr;
            }
            if (hop.list)
            {
                if (!schema->is(rose::SchemaType::List))
                {
                    return nullptr;
                }
                schema = schema->getElementSchema();
                continue;
            }
            if (!schema->is(rose::SchemaType::Struct))
            {
                return nullptr;
            }
            Schema::Member* member = schema->getMember(hop.member);
            if (member == nullptr)
            {
                return nullptr;
            }
            schema = member->schema;
        }
        return schema;
    }

    NestedField TableDocument::instanceLeafAtPath(const std::vector<NestedHop>& path) const
    {
        if (path.empty())
        {
            return this->instanceRootField();
        }
        std::vector<NestedHop> parent(path.begin(), path.end() - 1);
        Schema* parentSchema = this->instanceSchemaAtPath(parent);
        if (parentSchema == nullptr)
        {
            return NestedField();
        }
        const NestedHop& hop = path.back();
        if (hop.list)
        {
            if (!parentSchema->is(rose::SchemaType::List))
            {
                return NestedField();
            }
            NestedField field = this->fieldFromStorage(parentSchema->getElementSchema(), String::empty,
                String::format("Item %u", hop.index + 1));
            field.listItem = true;
            return field;
        }
        if (!parentSchema->is(rose::SchemaType::Struct))
        {
            return NestedField();
        }
        Schema::Member* member = parentSchema->getMember(hop.member);
        if (member == nullptr)
        {
            return NestedField();
        }
        return this->fieldFromStorage(member->schema, member->name, member->name);
    }

    rose::Value* TableDocument::instanceValueAtPath(const std::vector<NestedHop>& path) const
    {
        Value* current = this->activeInstanceValue();
        if (current == nullptr)
        {
            return nullptr;
        }
        for (const NestedHop& hop : path)
        {
            if (current == nullptr)
            {
                return nullptr;
            }
            if (hop.list)
            {
                if (current->schema == nullptr || !current->schema->is(rose::SchemaType::List))
                {
                    return nullptr;
                }
                try
                {
                    current = mLibrary->get(current, hop.index);
                }
                catch (const std::out_of_range&)
                {
                    return nullptr;
                }
                continue;
            }
            current = this->fieldAt(current, hop.member);
        }
        return current;
    }

    rose::Value* TableDocument::instanceEnsureAtPath(const std::vector<NestedHop>& path)
    {
        Value* root = this->activeInstanceValue();
        if (!mOpen || root == nullptr || root->schema == nullptr || mLibrary == nullptr)
        {
            return nullptr;
        }
        auto ensureMember = [this](Value* parent, const String& field, Schema* want) -> Value*
        {
            if (parent == nullptr || want == nullptr)
            {
                return nullptr;
            }
            Value* slot = this->fieldAt(parent, field);
            if (slot == nullptr || slot->schema == nullptr || slot->schema->is(rose::SchemaType::None)
                || slot->schema != want)
            {
                Value* made = mLibrary->make(want);
                if (made == nullptr)
                {
                    return nullptr;
                }
                mLibrary->set(parent, field, made);
                return this->fieldAt(parent, field);
            }
            return slot;
        };
        Value* current = root;
        Schema* walk = root->schema;
        for (const NestedHop& hop : path)
        {
            if (current == nullptr || walk == nullptr)
            {
                return nullptr;
            }
            if (hop.list)
            {
                if (!walk->is(rose::SchemaType::List))
                {
                    return nullptr;
                }
                Schema* element = walk->getElementSchema();
                Value* item = nullptr;
                try
                {
                    item = mLibrary->get(current, hop.index);
                }
                catch (const std::out_of_range&)
                {
                    item = nullptr;
                }
                if (item == nullptr || item->schema == nullptr || item->schema->is(rose::SchemaType::None)
                    || item->schema != element)
                {
                    Value* made = mLibrary->make(element);
                    if (made == nullptr)
                    {
                        return nullptr;
                    }
                    u32_t count = mLibrary->count(current);
                    if (hop.index >= count)
                    {
                        mLibrary->push(current, made);
                    }
                    else
                    {
                        mLibrary->set(current, hop.index, made);
                    }
                    try
                    {
                        item = mLibrary->get(current, hop.index);
                    }
                    catch (const std::out_of_range&)
                    {
                        return nullptr;
                    }
                }
                current = item;
                walk = element;
                continue;
            }
            if (!walk->is(rose::SchemaType::Struct))
            {
                return nullptr;
            }
            Schema::Member* member = walk->getMember(hop.member);
            if (member == nullptr)
            {
                return nullptr;
            }
            current = ensureMember(current, hop.member, member->schema);
            walk = member->schema;
        }
        return current;
    }

    rose::Value* TableDocument::instanceEnsureParentAtPath(const std::vector<NestedHop>& path)
    {
        if (path.empty())
        {
            return this->activeInstanceValue();
        }
        std::vector<NestedHop> parent(path.begin(), path.end() - 1);
        return this->instanceEnsureAtPath(parent);
    }

    u32_t TableDocument::instanceStructMemberCount() const
    {
        Value* value = this->activeInstanceValue();
        if (value == nullptr || value->schema == nullptr || !value->schema->is(rose::SchemaType::Struct))
        {
            return 0;
        }
        return value->schema->getMemberCount();
    }

    NestedField TableDocument::instanceStructMember(u32_t memberIndex) const
    {
        Value* value = this->activeInstanceValue();
        if (value == nullptr || value->schema == nullptr || !value->schema->is(rose::SchemaType::Struct)
            || memberIndex >= value->schema->getMemberCount())
        {
            return NestedField();
        }
        Schema::Member* member = value->schema->getMember(memberIndex);
        if (member == nullptr)
        {
            return NestedField();
        }
        return this->fieldFromStorage(member->schema, member->name, member->name);
    }

    u32_t TableDocument::instanceNestedFieldCount(const std::vector<NestedHop>& path) const
    {
        Schema* schema = this->instanceSchemaAtPath(path);
        if (schema == nullptr || !schema->is(rose::SchemaType::Struct))
        {
            return 0;
        }
        return schema->getMemberCount();
    }

    NestedField TableDocument::instanceNestedField(const std::vector<NestedHop>& path, u32_t index) const
    {
        Schema* schema = this->instanceSchemaAtPath(path);
        if (schema == nullptr || !schema->is(rose::SchemaType::Struct) || index >= schema->getMemberCount())
        {
            return NestedField();
        }
        Schema::Member* member = schema->getMember(index);
        if (member == nullptr)
        {
            return NestedField();
        }
        return this->fieldFromStorage(member->schema, member->name, member->name);
    }

    NestedField TableDocument::instanceNestedListElement(const std::vector<NestedHop>& path) const
    {
        Schema* schema = this->instanceSchemaAtPath(path);
        if (schema == nullptr || !schema->is(rose::SchemaType::List))
        {
            return NestedField();
        }
        return this->fieldFromStorage(schema->getElementSchema(), String::empty, String::empty);
    }

    u32_t TableDocument::instanceNestedListCount(const std::vector<NestedHop>& path) const
    {
        Value* list = this->instanceValueAtPath(path);
        if (list == nullptr || list->schema == nullptr || !list->schema->is(rose::SchemaType::List))
        {
            return 0;
        }
        return mLibrary->count(list);
    }

    String TableDocument::instanceStructText(const std::vector<NestedHop>& path) const
    {
        if (path.empty())
        {
            return this->instanceText();
        }
        return this->textFromValue(this->instanceValueAtPath(path), this->instanceLeafAtPath(path));
    }

    bool TableDocument::setInstanceStructText(const std::vector<NestedHop>& path, const String& text)
    {
        if (!mOpen || path.empty())
        {
            return false;
        }
        NestedField leaf = this->instanceLeafAtPath(path);
        Value* parent = this->instanceEnsureParentAtPath(path);
        if (parent == nullptr)
        {
            return false;
        }
        String before = this->instanceStructText(path);
        if (!this->writeFieldValue(parent, path.back(), leaf, trimCopy(text)))
        {
            return false;
        }
        if (before != this->instanceStructText(path))
        {
            this->markDirty();
        }
        return true;
    }

    std::vector<String> TableDocument::instanceStructLabels(const std::vector<NestedHop>& path) const
    {
        if (path.empty())
        {
            return this->instanceLabels();
        }
        return this->labelsFromValue(this->instanceValueAtPath(path), this->instanceLeafAtPath(path));
    }

    bool TableDocument::setInstanceStructLabels(const std::vector<NestedHop>& path, const std::vector<String>& labels)
    {
        if (!mOpen || path.empty())
        {
            return false;
        }
        NestedField leaf = this->instanceLeafAtPath(path);
        Value* parent = this->instanceEnsureParentAtPath(path);
        if (parent == nullptr)
        {
            return false;
        }
        this->writeFieldLabels(parent, path.back(), leaf, labels);
        this->markDirty();
        return true;
    }

    bool TableDocument::instanceStructListInsert(const std::vector<NestedHop>& path, u32_t index)
    {
        Schema* schema = this->instanceSchemaAtPath(path);
        if (schema == nullptr || !schema->is(rose::SchemaType::List))
        {
            return false;
        }
        Schema* element = schema->getElementSchema();
        if (element == nullptr)
        {
            return false;
        }
        Value* list = this->instanceEnsureAtPath(path);
        if (list == nullptr)
        {
            return false;
        }
        Value* item = mLibrary->make(element);
        if (item == nullptr)
        {
            return false;
        }
        u32_t count = mLibrary->count(list);
        if (index >= count)
        {
            mLibrary->push(list, item);
        }
        else
        {
            mLibrary->insert(list, index, item);
        }
        this->markDirty();
        return true;
    }

    bool TableDocument::instanceStructListRemove(const std::vector<NestedHop>& path, u32_t index)
    {
        Value* list = this->instanceValueAtPath(path);
        if (list == nullptr || list->schema == nullptr || !list->schema->is(rose::SchemaType::List))
        {
            return false;
        }
        if (index >= mLibrary->count(list))
        {
            return false;
        }
        mLibrary->remove(list, index);
        this->markDirty();
        return true;
    }

    u32_t TableDocument::instanceListCount() const
    {
        Value* value = this->activeInstanceValue();
        if (value == nullptr || value->schema == nullptr || !value->schema->is(rose::SchemaType::List))
        {
            return 0;
        }
        return mLibrary->count(value);
    }

    bool TableDocument::instanceListInsert(u32_t index)
    {
        Value* root = this->activeInstanceValue();
        if (!mOpen || root == nullptr || root->schema == nullptr || !root->schema->is(rose::SchemaType::List)
            || mLibrary == nullptr)
        {
            return false;
        }
        Schema* element = root->schema->getElementSchema();
        if (element == nullptr)
        {
            return false;
        }
        Value* item = mLibrary->make(element);
        if (item == nullptr)
        {
            return false;
        }
        u32_t count = mLibrary->count(root);
        if (index >= count)
        {
            mLibrary->push(root, item);
        }
        else
        {
            mLibrary->insert(root, index, item);
        }
        this->markDirty();
        return true;
    }

    bool TableDocument::instanceListRemove(u32_t index)
    {
        Value* root = this->activeInstanceValue();
        if (root == nullptr || root->schema == nullptr || !root->schema->is(rose::SchemaType::List))
        {
            return false;
        }
        if (index >= mLibrary->count(root))
        {
            return false;
        }
        mLibrary->remove(root, index);
        this->markDirty();
        return true;
    }

    u32_t TableDocument::instanceListColumnCount() const
    {
        Value* root = this->activeInstanceValue();
        if (root == nullptr || root->schema == nullptr || !root->schema->is(rose::SchemaType::List))
        {
            return 0;
        }
        Schema* element = root->schema->getElementSchema();
        if (element == nullptr)
        {
            return 0;
        }
        if (element->is(rose::SchemaType::Struct))
        {
            return element->getMemberCount();
        }
        return 1;
    }

    NestedField TableDocument::instanceListColumn(u32_t col) const
    {
        Value* root = this->activeInstanceValue();
        if (root == nullptr || root->schema == nullptr || !root->schema->is(rose::SchemaType::List))
        {
            return NestedField();
        }
        Schema* element = root->schema->getElementSchema();
        if (element == nullptr)
        {
            return NestedField();
        }
        if (element->is(rose::SchemaType::Struct))
        {
            if (col >= element->getMemberCount())
            {
                return NestedField();
            }
            Schema::Member* member = element->getMember(col);
            if (member == nullptr)
            {
                return NestedField();
            }
            return this->fieldFromStorage(member->schema, member->name, member->name);
        }
        if (col > 0)
        {
            return NestedField();
        }
        return this->fieldFromStorage(element, String::empty, String("Value"));
    }

    String TableDocument::instanceListCellText(u32_t row, u32_t col) const
    {
        Value* root = this->activeInstanceValue();
        if (root == nullptr || row >= this->instanceListCount())
        {
            return String::empty;
        }
        Value* item = nullptr;
        try
        {
            item = mLibrary->get(root, row);
        }
        catch (const std::out_of_range&)
        {
            return String::empty;
        }
        NestedField info = this->instanceListColumn(col);
        if (item == nullptr || item->schema == nullptr)
        {
            return String::empty;
        }
        Schema* element = root->schema->getElementSchema();
        if (element != nullptr && element->is(rose::SchemaType::Struct))
        {
            if (col >= element->getMemberCount())
            {
                return String::empty;
            }
            Schema::Member* member = element->getMember(col);
            if (member == nullptr)
            {
                return String::empty;
            }
            item = this->fieldAt(item, member->name);
            info = this->fieldFromStorage(member->schema, member->name, member->name);
        }
        return this->textFromValue(item, info);
    }

    bool TableDocument::setInstanceListCellText(u32_t row, u32_t col, const String& text)
    {
        Value* root = this->activeInstanceValue();
        if (!mOpen || root == nullptr || root->schema == nullptr || !root->schema->is(rose::SchemaType::List)
            || row >= this->instanceListCount())
        {
            return false;
        }
        Value* item = nullptr;
        try
        {
            item = mLibrary->get(root, row);
        }
        catch (const std::out_of_range&)
        {
            return false;
        }
        if (item == nullptr)
        {
            return false;
        }
        Schema* element = root->schema->getElementSchema();
        NestedField info = this->instanceListColumn(col);
        if (element != nullptr && element->is(rose::SchemaType::Struct))
        {
            if (col >= element->getMemberCount())
            {
                return false;
            }
            Schema::Member* member = element->getMember(col);
            if (member == nullptr)
            {
                return false;
            }
            if (item->schema == nullptr || item->schema->is(rose::SchemaType::None) || item->schema != element)
            {
                Value* made = mLibrary->make(element);
                if (made == nullptr)
                {
                    return false;
                }
                mLibrary->set(root, row, made);
                item = mLibrary->get(root, row);
            }
            info = this->fieldFromStorage(member->schema, member->name, member->name);
            if (!this->writeFieldValue(item, NestedHop::field(member->name), info, trimCopy(text)))
            {
                return false;
            }
        }
        else if (col == 0)
        {
            if (!this->writeFieldValue(root, NestedHop::at(row), info, trimCopy(text)))
            {
                return false;
            }
        }
        else
        {
            return false;
        }
        this->markDirty();
        return true;
    }

    std::vector<String> TableDocument::instanceListCellLabels(u32_t row, u32_t col) const
    {
        Value* root = this->activeInstanceValue();
        if (root == nullptr || row >= this->instanceListCount())
        {
            return {};
        }
        Value* item = nullptr;
        try
        {
            item = mLibrary->get(root, row);
        }
        catch (const std::out_of_range&)
        {
            return {};
        }
        NestedField info = this->instanceListColumn(col);
        if (item == nullptr || item->schema == nullptr)
        {
            return {};
        }
        Schema* element = root->schema->getElementSchema();
        if (element != nullptr && element->is(rose::SchemaType::Struct))
        {
            if (col >= element->getMemberCount())
            {
                return {};
            }
            Schema::Member* member = element->getMember(col);
            if (member == nullptr)
            {
                return {};
            }
            item = this->fieldAt(item, member->name);
            info = this->fieldFromStorage(member->schema, member->name, member->name);
        }
        return this->labelsFromValue(item, info);
    }

    bool TableDocument::setInstanceListCellLabels(u32_t row, u32_t col, const std::vector<String>& labels)
    {
        Value* root = this->activeInstanceValue();
        if (!mOpen || root == nullptr || root->schema == nullptr || !root->schema->is(rose::SchemaType::List)
            || row >= this->instanceListCount())
        {
            return false;
        }
        Value* item = nullptr;
        try
        {
            item = mLibrary->get(root, row);
        }
        catch (const std::out_of_range&)
        {
            return false;
        }
        if (item == nullptr)
        {
            return false;
        }
        Schema* element = root->schema->getElementSchema();
        NestedField info = this->instanceListColumn(col);
        if (element != nullptr && element->is(rose::SchemaType::Struct))
        {
            if (col >= element->getMemberCount())
            {
                return false;
            }
            Schema::Member* member = element->getMember(col);
            if (member == nullptr)
            {
                return false;
            }
            if (item->schema == nullptr || item->schema->is(rose::SchemaType::None) || item->schema != element)
            {
                Value* made = mLibrary->make(element);
                if (made == nullptr)
                {
                    return false;
                }
                mLibrary->set(root, row, made);
                item = mLibrary->get(root, row);
            }
            info = this->fieldFromStorage(member->schema, member->name, member->name);
            this->writeFieldLabels(item, NestedHop::field(member->name), info, labels);
        }
        else if (col == 0)
        {
            this->writeFieldLabels(root, NestedHop::at(row), info, labels);
        }
        else
        {
            return false;
        }
        this->markDirty();
        return true;
    }

}
