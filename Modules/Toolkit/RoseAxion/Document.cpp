#include "./Document.h"
#include "./BuildinSchema.h"

#include <functional>
#include <set>
#include <stdexcept>
#include <utility>

namespace eokas::RoseAxion {

    using rose::Schema;
    using rose::Value;

    namespace {

        String columnLabel(u32_t index)
        {
            String label = String::empty;
            u32_t number = index + 1;
            while (number > 0)
            {
                u32_t rem = (number - 1) % 26;
                label = String(static_cast<char>('A' + rem)) + label;
                number = (number - 1) / 26;
            }
            return label;
        }

        const float kDefaultColumnWidth = 168.0f;
        const float kDefaultHeadWidth = 84.0f;
        const float kMinColumnWidth = 48.0f;

        float clampWidth(float width)
        {
            if (width < kMinColumnWidth)
            {
                return kMinColumnWidth;
            }
            return width;
        }

        bool parseSuffixU32(const String& text, const String& prefix, u32_t& out)
        {
            if (!text.startsWith(prefix) || text.length() <= prefix.length())
            {
                return false;
            }
            u32_t number = 0;
            for (size_t i = prefix.length(); i < text.length(); ++i)
            {
                char c = text.at(i);
                if (c < '0' || c > '9')
                {
                    return false;
                }
                number = number * 10 + static_cast<u32_t>(c - '0');
            }
            out = number;
            return true;
        }

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

        const char* optionNameConflict(const std::vector<FieldOption>& options)
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
                    return "选项名称已存在";
                }
                seen.push_back(name);
            }
            return nullptr;
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
                if (option.name == stored || option.label == stored)
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
                if (option.label == labelOrName || option.name == labelOrName)
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
                item.label = option->label.isEmpty() ? option->name : option->label;
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
                String label = option.label.isEmpty() ? option.name : option.label;
                if (!schema->addOption(option.name, label, schemaColorFromHex(option.color)))
                {
                    return false;
                }
            }
            return true;
        }

    }

    void TableDocument::createNew()
    {
        mLibrary = std::make_unique<rose::Library>("table");
        this->clearActive();
        mTables = nullptr;
        mNextTableId = 0;
        mActiveSchema = -1;
        if (!ensureBuildinSchemas(*mLibrary) || !this->ensureSharedSchemas(true))
        {
            mLibrary.reset();
            mOpen = false;
            mStatus = "无法创建表格";
            return;
        }
        mTables = mLibrary->make(mTablesSchema);
        mLibrary->set("tables", mTables);
        mPath = String::empty;
        mOpen = true;
        mDirty = false;
        this->addTable();
        mDirty = false;
        mStatus = String::empty;
    }

    bool TableDocument::load(const String& path)
    {
        auto next = std::make_unique<rose::Library>("table");
        if (!next->load(path))
        {
            mStatus = "打开失败";
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
                mTables = nullptr;
                mOpen = false;
            }
            mStatus = "这不是 RoseAxion 表格";
            return false;
        }
        mPath = path;
        mStatus = "已打开";
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
            mStatus = "保存失败";
            return false;
        }
        mPath = path;
        mDirty = false;
        mStatus = "已保存";
        return true;
    }

    bool TableDocument::bind()
    {
        mOpen = false;
        this->clearActive();
        mTables = nullptr;
        mActiveSchema = -1;
        if (mLibrary == nullptr)
        {
            return false;
        }
        if (!this->ensureSharedSchemas(false))
        {
            return false;
        }
        mTables = mLibrary->get("tables");
        if (mTables == nullptr)
        {
            return false;
        }
        else if (mTables->schema == nullptr || !mTables->schema->is(rose::SchemaType::List))
        {
            return false;
        }
        this->refreshTableIds();
        mOpen = true;
        if (this->tableCount() > 0)
        {
            if (!this->bindTable(this->tableValue(0), 0))
            {
                mOpen = false;
                this->clearActive();
                return false;
            }
            mDirty = false;
        }
        else
        {
            mDirty = false;
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
        auto require = [this, install](const char* name, rose::SchemaType type) -> Schema*
        {
            Schema* schema = mLibrary->getSchema(name);
            if (schema == nullptr)
            {
                if (!install)
                {
                    return nullptr;
                }
                schema = mLibrary->addSchema(type, String(name));
            }
            return schema;
        };
        mColumnSchema = require("Column", rose::SchemaType::Struct);
        if (mColumnSchema == nullptr)
        {
            return false;
        }
        if (install && mColumnSchema->getMemberCount() == 0)
        {
            if (!mColumnSchema->addMember("id", mString)
                || !mColumnSchema->addMember("name", mString)
                || !mColumnSchema->addMember("kind", mString)
                || !mColumnSchema->addMember("width", mFloat))
            {
                return false;
            }
        }
        mColumnsSchema = require("Columns", rose::SchemaType::List);
        if (mColumnsSchema == nullptr)
        {
            return false;
        }
        if (install)
        {
            mColumnsSchema->setElementSchema(mColumnSchema);
        }
        rose::Schema* tableRows = require("TableRows", rose::SchemaType::List);
        if (tableRows == nullptr)
        {
            return false;
        }
        // ROSE save writes List element index; actual table row data uses per-table Rows_* lists.
        if (install && tableRows->getElementSchema() == nullptr)
        {
            tableRows->setElementSchema(mString);
        }
        mTableSchema = require("Table", rose::SchemaType::Struct);
        if (mTableSchema == nullptr)
        {
            return false;
        }
        if (install && mTableSchema->getMemberCount() == 0)
        {
            if (!mTableSchema->addMember("id", mString)
                || !mTableSchema->addMember("name", mString)
                || !mTableSchema->addMember("rowSchema", mString)
                || !mTableSchema->addMember("columns", mColumnsSchema)
                || !mTableSchema->addMember("rows", tableRows)
                || !mTableSchema->addMember("headWidth", mFloat))
            {
                return false;
            }
        }
        mTablesSchema = require("Tables", rose::SchemaType::List);
        if (mTablesSchema == nullptr)
        {
            return false;
        }
        if (install)
        {
            mTablesSchema->setElementSchema(mTableSchema);
        }
        return mColumnSchema != nullptr
            && mColumnsSchema != nullptr
            && mTableSchema != nullptr
            && mTablesSchema != nullptr
            && tableRows != nullptr;
    }

    bool TableDocument::bindTable(rose::Value* table, int index)
    {
        if (table == nullptr || mLibrary == nullptr)
        {
            return false;
        }
        String rowName = this->readString(table, "rowSchema");
        Schema* rowSchema = mLibrary->getSchema(rowName);
        Value* columns = this->fieldAt(table, "columns");
        Value* rows = this->fieldAt(table, "rows");
        if (rowSchema == nullptr || columns == nullptr || rows == nullptr)
        {
            return false;
        }
        if (columns->schema == nullptr || rows->schema == nullptr)
        {
            return false;
        }
        if (!columns->schema->is(rose::SchemaType::List) || !rows->schema->is(rose::SchemaType::List))
        {
            return false;
        }
        mActiveTable = table;
        mActiveIndex = index;
        mRowSchema = rowSchema;
        mColumns = columns;
        mRows = rows;
        this->refreshIds();
        return true;
    }

    void TableDocument::clearActive()
    {
        mActiveTable = nullptr;
        mActiveIndex = -1;
        mRowSchema = nullptr;
        mColumns = nullptr;
        mRows = nullptr;
        mNextId = 0;
    }

    u32_t TableDocument::tableCount() const
    {
        if (mLibrary == nullptr || mTables == nullptr)
        {
            return 0;
        }
        return mLibrary->count(mTables);
    }

    bool TableDocument::hasActiveTable() const
    {
        return mOpen && mLibrary != nullptr && mActiveTable != nullptr && mColumns != nullptr && mRows != nullptr && mRowSchema != nullptr;
    }

    rose::Value* TableDocument::tableValue(u32_t index) const
    {
        if (mLibrary == nullptr || mTables == nullptr)
        {
            return nullptr;
        }
        return mLibrary->get(mTables, index);
    }

    String TableDocument::tableId(u32_t index) const
    {
        return this->readString(this->tableValue(index), "id");
    }

    String TableDocument::tableName(u32_t index) const
    {
        String name = this->readString(this->tableValue(index), "name");
        if (name.isEmpty())
        {
            return this->tableId(index);
        }
        return name;
    }

    bool TableDocument::selectTable(u32_t index)
    {
        if (!mOpen || index >= this->tableCount())
        {
            return false;
        }
        if (mActiveIndex == static_cast<int>(index) && this->hasActiveTable())
        {
            return true;
        }
        if (!this->bindTable(this->tableValue(index), static_cast<int>(index)))
        {
            return false;
        }
        if (this->rebuildRows(false))
        {
            mDirty = true;
        }
        return true;
    }

    String TableDocument::nextNewTableName() const
    {
        for (u32_t number = 1; ; ++number)
        {
            String name = String::format("NewTable-%u", number);
            if (!this->tableNameTaken(name, -1))
            {
                return name;
            }
        }
    }

    String TableDocument::makeTableId()
    {
        for (;;)
        {
            String id = String::format("t%u", mNextTableId);
            ++mNextTableId;
            bool used = false;
            u32_t count = this->tableCount();
            for (u32_t index = 0; index < count; ++index)
            {
                if (this->tableId(index) == id)
                {
                    used = true;
                    break;
                }
            }
            if (!used)
            {
                return id;
            }
        }
    }

    bool TableDocument::tableNameTaken(const String& name, int skipIndex) const
    {
        u32_t count = this->tableCount();
        for (u32_t index = 0; index < count; ++index)
        {
            if (skipIndex >= 0 && static_cast<int>(index) == skipIndex)
            {
                continue;
            }
            if (this->tableName(index) == name)
            {
                return true;
            }
        }
        return false;
    }

    void TableDocument::refreshTableIds()
    {
        mNextTableId = 0;
        auto consider = [this](const String& text, const String& prefix)
        {
            u32_t number = 0;
            if (parseSuffixU32(text, prefix, number) && number + 1 > mNextTableId)
            {
                mNextTableId = number + 1;
            }
        };
        u32_t count = this->tableCount();
        for (u32_t index = 0; index < count; ++index)
        {
            consider(this->tableId(index), "t");
        }
        if (mLibrary == nullptr)
        {
            return;
        }
        u32_t schemas = mLibrary->getSchemaCount();
        for (u32_t index = 0; index < schemas; ++index)
        {
            Schema* schema = mLibrary->getSchema(index);
            if (schema == nullptr)
            {
                continue;
            }
            consider(schema->name(), "Row_t");
            consider(schema->name(), "Rows_t");
        }
    }

    bool TableDocument::addTable()
    {
        if (!mOpen || mLibrary == nullptr || mTables == nullptr || mTableSchema == nullptr || mColumnsSchema == nullptr)
        {
            return false;
        }
        String id = this->makeTableId();
        String name = this->nextNewTableName();
        String rowName = String("Row_") + id;
        String rowsName = String("Rows_") + id;
        Schema* rowSchema = mLibrary->addSchema(rose::SchemaType::Struct, rowName);
        if (rowSchema == nullptr)
        {
            rowSchema = mLibrary->getSchema(rowName);
        }
        Schema* rowsSchema = mLibrary->addSchema(rose::SchemaType::List, rowsName);
        if (rowsSchema == nullptr)
        {
            rowsSchema = mLibrary->getSchema(rowsName);
        }
        if (rowSchema == nullptr || rowsSchema == nullptr)
        {
            return false;
        }
        while (rowSchema->getMemberCount() > 0)
        {
            Schema::Member* member = rowSchema->getMember(0);
            if (member == nullptr)
            {
                break;
            }
            rowSchema->delMember(member->name);
        }
        rowsSchema->setElementSchema(rowSchema);
        Value* table = mLibrary->make(mTableSchema);
        Value* columns = mLibrary->make(mColumnsSchema);
        Value* rows = mLibrary->make(rowsSchema);
        if (table == nullptr || columns == nullptr || rows == nullptr)
        {
            return false;
        }
        mLibrary->set(table, "id", id);
        mLibrary->set(table, "name", name);
        mLibrary->set(table, "rowSchema", rowName);
        mLibrary->set(table, "columns", columns);
        mLibrary->set(table, "rows", rows);
        mLibrary->set(table, "headWidth", static_cast<f64_t>(kDefaultHeadWidth));
        if (!mLibrary->push(mTables, table))
        {
            return false;
        }
        u32_t index = this->tableCount() - 1;
        if (!this->bindTable(this->tableValue(index), static_cast<int>(index)))
        {
            mLibrary->remove(mTables, index);
            this->clearActive();
            return false;
        }
        this->insertColumn(0);
        mDirty = true;
        return true;
    }

    bool TableDocument::renameTable(u32_t index, const String& name)
    {
        if (!mOpen || index >= this->tableCount())
        {
            return false;
        }
        String next = name.trim();
        if (next.isEmpty())
        {
            mStatus = "表格名称不能为空";
            return false;
        }
        if (this->tableNameTaken(next, static_cast<int>(index)))
        {
            mStatus = "表格名称已存在";
            return false;
        }
        if (this->tableName(index) == next)
        {
            return true;
        }
        mLibrary->set(this->tableValue(index), "name", next);
        mDirty = true;
        return true;
    }

    bool TableDocument::deleteTable(u32_t index)
    {
        if (!mOpen || mLibrary == nullptr || mTables == nullptr || index >= this->tableCount())
        {
            return false;
        }
        if (!mLibrary->remove(mTables, index))
        {
            return false;
        }
        mDirty = true;
        u32_t count = this->tableCount();
        if (count == 0)
        {
            this->clearActive();
            return true;
        }
        int next = mActiveIndex;
        if (mActiveIndex == static_cast<int>(index))
        {
            next = static_cast<int>(index);
            if (next >= static_cast<int>(count))
            {
                next = static_cast<int>(count) - 1;
            }
        }
        else if (mActiveIndex > static_cast<int>(index))
        {
            next = mActiveIndex - 1;
        }
        if (next < 0)
        {
            this->clearActive();
            return true;
        }
        if (!this->bindTable(this->tableValue(static_cast<u32_t>(next)), next))
        {
            this->clearActive();
            return false;
        }
        this->rebuildRows(false);
        return true;
    }

    bool TableDocument::isScaffoldingSchemaName(const String& name) const
    {
        if (name == "None"
            || name == "Column" || name == "Columns" || name == "TableRows" || name == "Table" || name == "Tables")
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
            String name = String::format("模式%u", number);
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
        u32_t tables = this->tableCount();
        String name = schema->name();
        for (u32_t i = 0; i < tables; ++i)
        {
            if (this->readString(this->tableValue(i), "rowSchema") == name)
            {
                return true;
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
            mStatus = "无法创建模式";
            return false;
        }
        if (type != rose::SchemaType::Struct && type != rose::SchemaType::List
            && type != rose::SchemaType::Options)
        {
            mStatus = "无法创建模式";
            return false;
        }
        String name = this->nextNewSchemaName();
        Schema* schema = mLibrary->addSchema(type, name);
        if (schema == nullptr)
        {
            mStatus = "无法创建模式";
            return false;
        }
        if (type == rose::SchemaType::List)
        {
            schema->setElementSchema(mString);
        }
        mDirty = true;
        mStatus = String::empty;
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
            mStatus = "模式名称不能为空";
            return false;
        }
        if (this->isReservedSchemaName(next))
        {
            mStatus = "模式名称已存在";
            return false;
        }
        if (schema->name() == next)
        {
            return true;
        }
        if (mLibrary->getSchema(next) != nullptr)
        {
            mStatus = "模式名称已存在";
            return false;
        }
        String from = schema->name();
        if (!mLibrary->renameSchema(from, next))
        {
            mStatus = "模式名称已存在";
            return false;
        }
        u32_t tables = this->tableCount();
        for (u32_t i = 0; i < tables; ++i)
        {
            Value* table = this->tableValue(i);
            if (this->readString(table, "rowSchema") == from)
            {
                mLibrary->set(table, "rowSchema", next);
            }
        }
        mDirty = true;
        mStatus = String::empty;
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
            mStatus = "无法删除模式";
            return false;
        }
        String name = schema->name();
        if (!mLibrary->removeSchema(name))
        {
            mStatus = "无法删除模式";
            return false;
        }
        mDirty = true;
        mStatus = String::empty;
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

    String TableDocument::schemaMemberLabel(u32_t memberIndex) const
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
        return member->label;
    }

    String TableDocument::schemaMemberColor(u32_t memberIndex) const
    {
        Schema* schema = this->activeSchema();
        if (schema == nullptr || memberIndex >= schema->getMemberCount())
        {
            return String::empty;
        }
        Schema::Member* member = schema->getMember(memberIndex);
        if (member == nullptr || member->color == 0)
        {
            return String::empty;
        }
        return hexFromSchemaColor(member->color);
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
            mStatus = "无法添加成员";
            return false;
        }
        mDirty = true;
        mStatus = String::empty;
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
            mStatus = "成员名称不能为空";
            return false;
        }
        if (member->name == next)
        {
            return true;
        }
        if (schema->getMember(next) != nullptr)
        {
            mStatus = "成员名称已存在";
            return false;
        }
        member->name = next;
        mDirty = true;
        mStatus = String::empty;
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
        if (!schema->setMember(member->name, next, member->label, member->color))
        {
            return false;
        }
        mDirty = true;
        mStatus = String::empty;
        return true;
    }

    bool TableDocument::setSchemaMemberLabel(u32_t memberIndex, const String& label)
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
        if (member->label == label)
        {
            return true;
        }
        member->label = label;
        mDirty = true;
        mStatus = String::empty;
        return true;
    }

    bool TableDocument::setSchemaMemberColor(u32_t memberIndex, const String& hex)
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
        u32_t next = schemaColorFromHex(hex);
        if (member->color == next)
        {
            return true;
        }
        member->color = next;
        mDirty = true;
        mStatus = String::empty;
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
        mDirty = true;
        mStatus = String::empty;
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
        mDirty = true;
        mStatus = String::empty;
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
        mDirty = true;
        mStatus = String::empty;
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

    bool TableDocument::isNestedColumn(u32_t column) const
    {
        if (!mOpen || column >= this->columnCount())
        {
            return false;
        }
        return this->isNestedColumnType(this->column(column).schemaName);
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
        if (const char* error = optionNameConflict(options))
        {
            mStatus = error;
            return false;
        }
        if (!replaceSchemaOptions(schema, options))
        {
            mStatus = "无法更新选项";
            return false;
        }
        mDirty = true;
        mStatus = String::empty;
        return true;
    }

    void TableDocument::refreshIds()
    {
        mNextId = 0;
        u32_t count = this->columnCount();
        for (u32_t index = 0; index < count; ++index)
        {
            String id = this->column(index).id;
            if (id.length() < 2 || id.at(0) != 'c')
            {
                continue;
            }
            u32_t number = 0;
            bool digits = true;
            for (size_t i = 1; i < id.length(); ++i)
            {
                char c = id.at(i);
                if (c < '0' || c > '9')
                {
                    digits = false;
                    break;
                }
                number = number * 10 + static_cast<u32_t>(c - '0');
            }
            if (digits && number + 1 > mNextId)
            {
                mNextId = number + 1;
            }
        }
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

    rose::Schema* TableDocument::optionsSchemaForStorage(rose::Schema* storage) const
    {
        return optionsSchemaOf(storage);
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

    u32_t TableDocument::rowCount() const
    {
        if (!this->hasActiveTable())
        {
            return 0;
        }
        return mLibrary->count(mRows);
    }

    u32_t TableDocument::columnCount() const
    {
        if (!this->hasActiveTable())
        {
            return 0;
        }
        return mLibrary->count(mColumns);
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

    bool TableDocument::rebuildRows(bool replaceAll)
    {
        if (mLibrary == nullptr || mRows == nullptr || mRowSchema == nullptr)
        {
            return false;
        }
        u32_t rows = this->rowCount();
        u32_t members = mRowSchema->getMemberCount();
        Value* none = nullptr;
        bool replaced = false;
        for (u32_t row = 0; row < rows; ++row)
        {
            Value* current = this->rowValue(row);
            std::vector<Value> kept(members);
            std::vector<char> present(members, 0);
            bool incomplete = current == nullptr;
            if (current != nullptr)
            {
                for (u32_t memberIndex = 0; memberIndex < members; ++memberIndex)
                {
                    Schema::Member* member = mRowSchema->getMember(memberIndex);
                    if (member == nullptr)
                    {
                        incomplete = true;
                        continue;
                    }
                    Value* src = this->fieldAt(current, member->name);
                    if (src == nullptr)
                    {
                        incomplete = true;
                        continue;
                    }
                    kept[memberIndex] = *src;
                    present[memberIndex] = 1;
                }
            }
            if (!replaceAll && !incomplete)
            {
                continue;
            }
            Value* fresh = mLibrary->make(mRowSchema);
            if (fresh == nullptr)
            {
                continue;
            }
            for (u32_t memberIndex = 0; memberIndex < members; ++memberIndex)
            {
                Schema::Member* member = mRowSchema->getMember(memberIndex);
                if (member == nullptr)
                {
                    continue;
                }
                if (present[memberIndex] != 0)
                {
                    mLibrary->set(fresh, member->name, &kept[memberIndex]);
                }
                else
                {
                    if (none == nullptr)
                    {
                        none = this->makeNone();
                    }
                    if (none != nullptr)
                    {
                        mLibrary->set(fresh, member->name, none);
                    }
                }
            }
            if (mLibrary->set(mRows, row, fresh))
            {
                replaced = true;
            }
        }
        return replaced;
    }

    rose::Value* TableDocument::columnValue(u32_t index) const
    {
        return mLibrary->get(mColumns, index);
    }

    rose::Value* TableDocument::rowValue(u32_t index) const
    {
        return mLibrary->get(mRows, index);
    }

    String TableDocument::readString(rose::Value* object, const char* field) const
    {
        Value* slot = this->fieldAt(object, field);
        if (slot == nullptr || slot->schema == nullptr || slot->schema->is(rose::SchemaType::None))
        {
            return String::empty;
        }
        String value;
        if (!mLibrary->get(slot, value))
        {
            return String::empty;
        }
        return value;
    }

    TableColumn TableDocument::column(u32_t index) const
    {
        TableColumn column;
        Value* object = this->columnValue(index);
        column.id = this->readString(object, "id");
        column.name = this->readString(object, "name");
        column.schemaName = canonicalizeColumnType(this->readString(object, "kind"));
        column.kind = this->fieldKindForColumnType(column.schemaName);
        column.width = this->readFloat(object, "width", kDefaultColumnWidth);
        if (mLibrary != nullptr)
        {
            column.options = fieldOptionsFromSchema(
                optionsSchemaOf(mLibrary->getSchema(column.schemaName)));
        }
        return column;
    }

    float TableDocument::readFloat(rose::Value* object, const char* field, float fallback) const
    {
        Value* slot = this->fieldAt(object, field);
        f64_t value = 0.0;
        if (slot == nullptr || !mLibrary->get(slot, value))
        {
            return fallback;
        }
        return static_cast<float>(value);
    }

    rose::Value* TableDocument::rebuildColumn(u32_t index, float width)
    {
        TableColumn info = this->column(index);
        Value* fresh = this->makeColumn(info.id, info.name, info.schemaName);
        mLibrary->set(fresh, "width", static_cast<f64_t>(clampWidth(width)));
        mLibrary->set(mColumns, index, fresh);
        return fresh;
    }

    float TableDocument::columnWidth(u32_t index) const
    {
        if (!mOpen || index >= this->columnCount())
        {
            return kDefaultColumnWidth;
        }
        return clampWidth(this->column(index).width);
    }

    bool TableDocument::setColumnWidth(u32_t index, float width)
    {
        if (!mOpen || index >= this->columnCount())
        {
            return false;
        }
        float next = clampWidth(width);
        if (this->columnWidth(index) == next)
        {
            return true;
        }
        if (!mLibrary->set(this->columnValue(index), "width", static_cast<f64_t>(next)))
        {
            this->rebuildColumn(index, next);
        }
        mDirty = true;
        return true;
    }

    float TableDocument::headWidth() const
    {
        if (!this->hasActiveTable())
        {
            return kDefaultHeadWidth;
        }
        return clampWidth(this->readFloat(mActiveTable, "headWidth", kDefaultHeadWidth));
    }

    bool TableDocument::setHeadWidth(float width)
    {
        if (!this->hasActiveTable())
        {
            return false;
        }
        float next = clampWidth(width);
        if (this->headWidth() == next)
        {
            return true;
        }
        mLibrary->set(mActiveTable, "headWidth", static_cast<f64_t>(next));
        mDirty = true;
        return true;
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

    std::vector<String> TableDocument::readStoredOptionNames(u32_t row, u32_t column) const
    {
        std::vector<String> names;
        if (!mOpen || row >= this->rowCount() || column >= this->columnCount())
        {
            return names;
        }
        TableColumn info = this->column(column);
        Value* field = this->fieldAt(this->rowValue(row), info.id);
        if (field == nullptr || field->schema == nullptr || field->schema->is(rose::SchemaType::None))
        {
            return names;
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
            return names;
        }
        String value;
        if (mLibrary->get(field, value) && !value.isEmpty())
        {
            names.push_back(value);
        }
        return names;
    }

    void TableDocument::writeLabels(u32_t row, const TableColumn& column, const std::vector<String>& labels)
    {
        std::vector<String> filtered = this->filterLabels(labels, column.options);
        Value* made = nullptr;
        if (filtered.empty())
        {
            made = this->makeNone();
        }
        else
        {
            Schema* storage = this->storageForType(column.schemaName);
            made = mLibrary->make(storage);
            if (made != nullptr)
            {
                Schema* opts = optionsSchemaOf(storage);
                mLibrary->clear(made);
                for (const String& label : filtered)
                {
                    String name = optionStorageNameFromLabel(column.options, label);
                    if (name.isEmpty())
                    {
                        continue;
                    }
                    if (column.kind == FieldKind::SingleOption)
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
        mLibrary->set(this->rowValue(row), column.id, made);
    }

    bool TableDocument::cellIsNull(u32_t row, u32_t column) const
    {
        if (!mOpen || row >= this->rowCount() || column >= this->columnCount())
        {
            return true;
        }
        TableColumn info = this->column(column);
        Value* field = this->fieldAt(this->rowValue(row), info.id);
        if (field == nullptr || field->schema == nullptr || field->schema->is(rose::SchemaType::None))
        {
            return true;
        }
        if (field->schema->is(rose::SchemaType::List) && mLibrary->count(field) == 0)
        {
            return true;
        }
        return false;
    }

    String TableDocument::cellText(u32_t row, u32_t column) const
    {
        if (this->cellIsNull(row, column))
        {
            return String::empty;
        }
        TableColumn info = this->column(column);
        if (info.kind == FieldKind::MultiOption)
        {
            return joinSelected(this->cellLabels(row, column));
        }
        Value* field = this->fieldAt(this->rowValue(row), info.id);
        if (field == nullptr || field->schema == nullptr)
        {
            return String::empty;
        }
        if (this->isNestedColumnType(info.schemaName))
        {
            if (field->schema->is(rose::SchemaType::Struct))
            {
                return String::format("{%u}", field->schema->getMemberCount());
            }
            if (field->schema->is(rose::SchemaType::List))
            {
                return String::format("[%u]", mLibrary->count(field));
            }
        }
        if (field->schema->is(rose::SchemaType::Int))
        {
            i64_t value = 0;
            if (!mLibrary->get(field, value))
            {
                return String::empty;
            }
            return String::format("%lld", static_cast<long long>(value));
        }
        if (field->schema->is(rose::SchemaType::Float))
        {
            f64_t value = 0;
            if (!mLibrary->get(field, value))
            {
                return String::empty;
            }
            return String::format("%.15g", value);
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
        if (info.kind == FieldKind::SingleOption && !info.options.empty())
        {
            return optionDisplayLabel(info.options, value);
        }
        return value;
    }

    std::vector<String> TableDocument::cellLabels(u32_t row, u32_t column) const
    {
        if (!mOpen || row >= this->rowCount() || column >= this->columnCount())
        {
            return {};
        }
        TableColumn info = this->column(column);
        std::vector<String> labels;
        std::vector<String> names = this->readStoredOptionNames(row, column);
        for (const String& name : names)
        {
            labels.push_back(optionDisplayLabel(info.options, name));
        }
        return this->filterLabels(labels, info.options);
    }

    bool TableDocument::setCellLabels(u32_t row, u32_t column, const std::vector<String>& labels)
    {
        if (!mOpen || row >= this->rowCount() || column >= this->columnCount())
        {
            return false;
        }
        TableColumn info = this->column(column);
        String before = this->cellText(row, column);
        bool wasNull = this->cellIsNull(row, column);
        this->writeLabels(row, info, labels);
        String after = this->cellText(row, column);
        bool nowNull = this->cellIsNull(row, column);
        if (before != after || wasNull != nowNull)
        {
            mDirty = true;
        }
        return true;
    }

    bool TableDocument::dropOptionLabel(u32_t column, const String& label)
    {
        if (!mOpen || column >= this->columnCount() || label.isEmpty())
        {
            return false;
        }
        TableColumn info = this->column(column);
        String gone = optionStorageNameFromLabel(info.options, label);
        if (gone.isEmpty())
        {
            gone = label;
        }
        u32_t rows = this->rowCount();
        for (u32_t row = 0; row < rows; ++row)
        {
            std::vector<String> names = this->readStoredOptionNames(row, column);
            std::vector<String> kept;
            bool changed = false;
            for (const String& name : names)
            {
                if (name == gone)
                {
                    changed = true;
                    continue;
                }
                kept.push_back(optionDisplayLabel(info.options, name));
            }
            if (changed)
            {
                this->setCellLabels(row, column, kept);
            }
        }
        return true;
    }

    void TableDocument::putNull(rose::Value* row, const String& field, rose::Schema* schema)
    {
        Value* slot = this->fieldAt(row, field);
        Schema* none = this->noneSchema();
        if (slot == nullptr || none == nullptr)
        {
            return;
        }
        slot->schema = none;
        (void)schema;
    }

    void TableDocument::writeCell(u32_t row, const TableColumn& column, const String& text)
    {
        if (this->isNestedColumnType(column.schemaName))
        {
            Schema* schema = this->storageForType(column.schemaName);
            Value* made = nullptr;
            if (trimCopy(text).isEmpty())
            {
                made = this->makeNone();
            }
            else if (schema != nullptr)
            {
                made = mLibrary->make(schema);
            }
            if (made != nullptr)
            {
                mLibrary->set(this->rowValue(row), column.id, made);
            }
            return;
        }
        if (column.kind == FieldKind::MultiOption)
        {
            this->writeLabels(row, column, splitSelected(text));
            return;
        }
        String value = trimCopy(text);
        if (column.kind == FieldKind::SingleOption)
        {
            String name = optionStorageNameFromLabel(column.options, value);
            if (!name.isEmpty())
            {
                value = name;
            }
        }
        Schema* schema = this->storageForType(column.schemaName);
        Value* made = nullptr;
        if (value.isEmpty() || !validateField(column.kind, value))
        {
            made = this->makeNone();
        }
        else
        {
            made = mLibrary->make(schema);
            switch (column.kind)
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
                mLibrary->set(made, canonicalText(column.kind, value));
                break;
            }
        }
        if (made == nullptr)
        {
            return;
        }
        mLibrary->set(this->rowValue(row), column.id, made);
    }

    bool TableDocument::setCellText(u32_t row, u32_t column, const String& text)
    {
        if (!mOpen || row >= this->rowCount() || column >= this->columnCount())
        {
            return false;
        }
        String before = this->cellText(row, column);
        bool wasNull = this->cellIsNull(row, column);
        TableColumn info = this->column(column);
        String trimmed = trimCopy(text);
        bool valid = trimmed.isEmpty() || validateField(info.kind, trimmed);
        this->writeCell(row, info, trimmed);
        String after = this->cellText(row, column);
        bool nowNull = this->cellIsNull(row, column);
        if (before != after || wasNull != nowNull)
        {
            mDirty = true;
        }
        if (!valid)
        {
            mStatus = "内容与字段类型不符，已清空";
        }
        return valid;
    }

    bool TableDocument::setColumnName(u32_t column, const String& name)
    {
        if (!mOpen || column >= this->columnCount())
        {
            return false;
        }
        if (this->column(column).name == name)
        {
            return true;
        }
        mLibrary->set(this->columnValue(column), "name", name);
        mDirty = true;
        return true;
    }

    bool TableDocument::columnChangeClears(u32_t column, const String& typeName) const
    {
        if (!mOpen || column >= this->columnCount())
        {
            return false;
        }
        FieldKind kind = this->fieldKindForColumnType(typeName);
        if (this->isNestedColumnType(typeName))
        {
            u32_t rows = this->rowCount();
            for (u32_t row = 0; row < rows; ++row)
            {
                if (!this->cellIsNull(row, column))
                {
                    return true;
                }
            }
            return false;
        }
        u32_t rows = this->rowCount();
        for (u32_t row = 0; row < rows; ++row)
        {
            if (this->cellIsNull(row, column))
            {
                continue;
            }
            if (kind == FieldKind::SingleOption && this->cellLabels(row, column).size() > 1)
            {
                return true;
            }
            String text = this->cellText(row, column);
            if (!text.isEmpty() && !validateField(kind, text))
            {
                return true;
            }
        }
        return false;
    }

    bool TableDocument::setColumnType(u32_t column, const String& typeName)
    {
        if (!mOpen || column >= this->columnCount())
        {
            return false;
        }
        TableColumn info = this->column(column);
        String next = canonicalizeColumnType(typeName);
        if (info.schemaName == next)
        {
            return true;
        }
        std::vector<String> texts;
        u32_t rows = this->rowCount();
        texts.reserve(rows);
        for (u32_t row = 0; row < rows; ++row)
        {
            texts.push_back(this->cellText(row, column));
        }
        FieldKind kind = this->fieldKindForColumnType(next);
        Schema* storage = this->storageForType(next);
        if (storage == nullptr)
        {
            return false;
        }
        Schema::Member* member = mRowSchema->getMember(info.id);
        String label = member != nullptr ? member->label : String::empty;
        u32_t color = member != nullptr ? member->color : 0;
        if (!mRowSchema->setMember(info.id, storage, label, color))
        {
            return false;
        }
        mLibrary->set(this->columnValue(column), "kind", storage->name());
        if (kind != FieldKind::SingleOption && kind != FieldKind::MultiOption)
        {
            info.options.clear();
        }
        else if (info.options.empty())
        {
            info.options = fieldOptionsFromSchema(optionsSchemaOf(storage));
        }
        info.schemaName = storage->name();
        info.kind = kind;
        for (u32_t row = 0; row < rows; ++row)
        {
            if (kind == FieldKind::SingleOption && splitSelected(texts[row]).size() > 1)
            {
                this->writeCell(row, info, String::empty);
            }
            else
            {
                this->writeCell(row, info, texts[row]);
            }
        }
        mDirty = true;
        return true;
    }

    bool TableDocument::setOptions(u32_t column, const std::vector<FieldOption>& options)
    {
        if (!mOpen || column >= this->columnCount())
        {
            return false;
        }
        Schema* storage = this->storageForType(this->column(column).schemaName);
        Schema* schema = this->optionsSchemaForStorage(storage);
        if (schema == nullptr)
        {
            return false;
        }
        if (const char* error = optionNameConflict(options))
        {
            mStatus = error;
            return false;
        }
        if (!replaceSchemaOptions(schema, options))
        {
            mStatus = "无法更新选项";
            return false;
        }
        mDirty = true;
        mStatus = String::empty;
        return true;
    }

    String TableDocument::makeId()
    {
        String id = String::format("c%u", mNextId);
        ++mNextId;
        return id;
    }

    rose::Value* TableDocument::makeColumn(const String& id, const String& name, const String& schemaName)
    {
        Value* column = mLibrary->make(mColumnSchema);
        mLibrary->set(column, "id", id);
        mLibrary->set(column, "name", name);
        mLibrary->set(column, "kind", schemaName);
        mLibrary->set(column, "width", static_cast<f64_t>(kDefaultColumnWidth));
        return column;
    }

    void TableDocument::insertRow(u32_t index)
    {
        if (!this->hasActiveTable())
        {
            return;
        }
        if (index > this->rowCount())
        {
            index = this->rowCount();
        }
        Value* row = mLibrary->make(mRowSchema);
        u32_t columns = this->columnCount();
        for (u32_t column = 0; column < columns; ++column)
        {
            TableColumn info = this->column(column);
            this->putNull(row, info.id, this->storageForType(info.schemaName));
        }
        if (!mLibrary->insert(mRows, index, row))
        {
            return;
        }
        mDirty = true;
    }

    void TableDocument::deleteRow(u32_t index)
    {
        if (!this->hasActiveTable() || index >= this->rowCount())
        {
            return;
        }
        if (this->rowCount() <= 1)
        {
            u32_t columns = this->columnCount();
            for (u32_t column = 0; column < columns; ++column)
            {
                this->writeCell(0, this->column(column), String::empty);
            }
        }
        else if (!mLibrary->remove(mRows, index))
        {
            return;
        }
        mDirty = true;
    }

    void TableDocument::insertColumn(u32_t index)
    {
        if (!this->hasActiveTable())
        {
            return;
        }
        u32_t count = this->columnCount();
        if (index > count)
        {
            index = count;
        }
        String id = this->makeId();
        String name = columnLabel(count);
        if (!mRowSchema->addMember(id, mString))
        {
            return;
        }
        this->rebuildRows(false);
        Value* column = this->makeColumn(id, name, String("String"));
        mLibrary->insert(mColumns, index, column);
        if (this->rowCount() == 0)
        {
            this->insertRow(0);
        }
        mDirty = true;
    }

    void TableDocument::deleteColumn(u32_t index)
    {
        if (!this->hasActiveTable() || index >= this->columnCount())
        {
            return;
        }
        if (this->columnCount() <= 1)
        {
            TableColumn info = this->column(index);
            Schema::Member* member = mRowSchema->getMember(info.id);
            String label = member != nullptr ? member->label : String::empty;
            u32_t color = member != nullptr ? member->color : 0;
            if (!mRowSchema->setMember(info.id, mString, label, color))
            {
                return;
            }
            mLibrary->set(this->columnValue(index), "kind", String("String"));
            u32_t rows = this->rowCount();
            info.schemaName = String("String");
            info.kind = FieldKind::Text;
            info.options.clear();
            for (u32_t row = 0; row < rows; ++row)
            {
                this->writeCell(row, info, String::empty);
            }
            mDirty = true;
            return;
        }
        TableColumn info = this->column(index);
        mRowSchema->delMember(info.id);
        this->rebuildRows(true);
        mLibrary->remove(mColumns, index);
        mDirty = true;
    }

}
