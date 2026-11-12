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

    const char* docStatusText(DocStatus status)
    {
        switch (status)
        {
        case DocStatus::Opened: return "已打开";
        case DocStatus::Saved: return "已保存";
        case DocStatus::OpenFailed: return "打开失败";
        case DocStatus::CannotOpenFile: return "无法打开文件";
        case DocStatus::NotATable: return "这不是 RoseAxion 表格";
        case DocStatus::SaveFailed: return "保存失败";
        case DocStatus::CannotSaveFile: return "无法保存文件";
        case DocStatus::TableNameEmpty: return "表格名称不能为空";
        case DocStatus::TableNameTaken: return "表格名称已存在";
        case DocStatus::CannotCreateTable: return "无法创建表格";
        case DocStatus::SchemaNameEmpty: return "模式名称不能为空";
        case DocStatus::SchemaNameTaken: return "模式名称已存在";
        case DocStatus::CannotCreateSchema: return "无法创建模式";
        case DocStatus::CannotDeleteSchema: return "无法删除模式";
        case DocStatus::CannotAddMember: return "无法添加成员";
        case DocStatus::MemberNameEmpty: return "成员名称不能为空";
        case DocStatus::MemberNameTaken: return "成员名称已存在";
        case DocStatus::OptionNameEmpty: return "选项名称不能为空";
        case DocStatus::OptionNameTaken: return "选项名称已存在";
        case DocStatus::CannotUpdateOptions: return "无法更新选项";
        case DocStatus::TypeMismatchCleared: return "内容与字段类型不符，已清空";
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
        mTables = nullptr;
        mNextTableId = 0;
        mActiveSchema = -1;
        if (!ensureBuildinSchemas(*mLibrary) || !this->ensureSharedSchemas(true))
        {
            mLibrary.reset();
            mOpen = false;
            this->setStatus(DocStatus::CannotCreateTable);
            return;
        }
        mTables = mLibrary->make(mTablesSchema);
        mLibrary->set("tables", mTables);
        mPath = String::empty;
        mOpen = true;
        mDirty = false;
        this->addTable();
        mDirty = false;
        this->setStatus(DocStatus::Ready);
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
                mTables = nullptr;
                mOpen = false;
            }
            this->setStatus(DocStatus::NotATable);
            return false;
        }
        mPath = path;
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
            this->markDirty();
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
        this->markDirty();
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
            this->setStatus(DocStatus::TableNameEmpty);
            return false;
        }
        if (this->tableNameTaken(next, static_cast<int>(index)))
        {
            this->setStatus(DocStatus::TableNameTaken);
            return false;
        }
        if (this->tableName(index) == next)
        {
            return true;
        }
        mLibrary->set(this->tableValue(index), "name", next);
        this->markDirty();
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
        this->markDirty();
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
        u32_t tables = this->tableCount();
        for (u32_t i = 0; i < tables; ++i)
        {
            Value* table = this->tableValue(i);
            if (this->readString(table, "rowSchema") == from)
            {
                mLibrary->set(table, "rowSchema", next);
            }
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
        if (!schema->setMember(member->name, next, member->label, member->color))
        {
            return false;
        }
        this->markDirty();
        this->setStatus(DocStatus::Ready);
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
        this->markDirty();
        this->setStatus(DocStatus::Ready);
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

    bool TableDocument::isNestedColumn(u32_t column) const
    {
        if (!mOpen || column >= this->columnCount())
        {
            return false;
        }
        return this->isNestedColumnType(this->column(column).schemaName);
    }

    bool TableDocument::isStructType(const String& typeName) const
    {
        Schema* schema = this->storageForType(typeName);
        return schema != nullptr && schema->is(rose::SchemaType::Struct);
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

    bool TableDocument::isNestedStructColumn(u32_t column) const
    {
        return this->hasActiveTable() && column < this->columnCount()
            && this->isStructType(this->column(column).schemaName);
    }

    bool TableDocument::isNestedListColumn(u32_t column) const
    {
        return this->hasActiveTable() && column < this->columnCount()
            && this->isListType(this->column(column).schemaName);
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

    NestedField TableDocument::objectField(u32_t column) const
    {
        if (!this->hasActiveTable() || column >= this->columnCount())
        {
            return NestedField();
        }
        TableColumn info = this->column(column);
        NestedField field = this->fieldFromStorage(this->storageForType(info.schemaName), info.id, info.name);
        field.kind = info.kind;
        field.options = info.options;
        field.nestedStruct = this->isNestedStructColumn(column);
        field.nestedList = this->isNestedListColumn(column);
        return field;
    }

    rose::Schema* TableDocument::schemaAtPath(u32_t column, const std::vector<NestedHop>& path) const
    {
        if (!this->hasActiveTable() || column >= this->columnCount())
        {
            return nullptr;
        }
        Schema* schema = this->storageForType(this->column(column).schemaName);
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

    NestedField TableDocument::leafAtPath(u32_t column, const std::vector<NestedHop>& path) const
    {
        if (path.empty())
        {
            return this->objectField(column);
        }
        Schema* parent = this->schemaAtPath(column, std::vector<NestedHop>(path.begin(), path.end() - 1));
        if (parent == nullptr)
        {
            return NestedField();
        }
        const NestedHop& hop = path.back();
        if (hop.list)
        {
            if (!parent->is(rose::SchemaType::List))
            {
                return NestedField();
            }
            NestedField field = this->fieldFromStorage(parent->getElementSchema(), String::empty, String::format("第 %u 项", hop.index + 1));
            field.listItem = true;
            return field;
        }
        if (!parent->is(rose::SchemaType::Struct))
        {
            return NestedField();
        }
        Schema::Member* member = parent->getMember(hop.member);
        if (member == nullptr)
        {
            return NestedField();
        }
        String name = member->label.isEmpty() ? member->name : member->label;
        return this->fieldFromStorage(member->schema, member->name, name);
    }

    u32_t TableDocument::nestedFieldCount(u32_t column, const std::vector<NestedHop>& path) const
    {
        Schema* schema = this->schemaAtPath(column, path);
        if (schema == nullptr || !schema->is(rose::SchemaType::Struct))
        {
            return 0;
        }
        return schema->getMemberCount();
    }

    NestedField TableDocument::nestedField(u32_t column, const std::vector<NestedHop>& path, u32_t index) const
    {
        Schema* schema = this->schemaAtPath(column, path);
        if (schema == nullptr || !schema->is(rose::SchemaType::Struct) || index >= schema->getMemberCount())
        {
            return NestedField();
        }
        Schema::Member* member = schema->getMember(index);
        if (member == nullptr)
        {
            return NestedField();
        }
        String name = member->label.isEmpty() ? member->name : member->label;
        return this->fieldFromStorage(member->schema, member->name, name);
    }

    rose::Value* TableDocument::valueAtPath(u32_t row, u32_t column, const std::vector<NestedHop>& path) const
    {
        if (!this->hasActiveTable() || row >= this->rowCount() || column >= this->columnCount())
        {
            return nullptr;
        }
        Value* current = this->fieldAt(this->rowValue(row), this->column(column).id);
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

    rose::Value* TableDocument::ensureAtPath(u32_t row, u32_t column, const std::vector<NestedHop>& path)
    {
        if (!this->hasActiveTable() || row >= this->rowCount() || column >= this->columnCount() || mLibrary == nullptr)
        {
            return nullptr;
        }
        TableColumn info = this->column(column);
        Schema* want = this->storageForType(info.schemaName);
        Value* rowValue = this->rowValue(row);
        auto ensureMember = [this](Value* parent, const String& field, Schema* want) -> Value*
        {
            if (parent == nullptr || want == nullptr)
            {
                return nullptr;
            }
            Value* slot = this->fieldAt(parent, field);
            if (slot == nullptr || slot->schema == nullptr || slot->schema->is(rose::SchemaType::None) || slot->schema != want)
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
        Value* current = ensureMember(rowValue, info.id, want);
        Schema* walk = want;
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
                if (item == nullptr || item->schema == nullptr || item->schema->is(rose::SchemaType::None) || item->schema != element)
                {
                    Value* made = mLibrary->make(element);
                    if (made == nullptr)
                    {
                        return nullptr;
                    }
                    mLibrary->set(current, hop.index, made);
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

    rose::Value* TableDocument::ensureParentAtPath(u32_t row, u32_t column, const std::vector<NestedHop>& path)
    {
        if (path.empty())
        {
            return this->rowValue(row);
        }
        return this->ensureAtPath(row, column, std::vector<NestedHop>(path.begin(), path.end() - 1));
    }

    u32_t TableDocument::nestedListCount(u32_t row, u32_t column, const std::vector<NestedHop>& path) const
    {
        Value* list = this->valueAtPath(row, column, path);
        if (list == nullptr || list->schema == nullptr || !list->schema->is(rose::SchemaType::List))
        {
            return 0;
        }
        return mLibrary->count(list);
    }

    NestedField TableDocument::nestedListElement(u32_t column, const std::vector<NestedHop>& path) const
    {
        Schema* schema = this->schemaAtPath(column, path);
        if (schema == nullptr || !schema->is(rose::SchemaType::List))
        {
            return NestedField();
        }
        return this->fieldFromStorage(schema->getElementSchema(), String::empty, String::empty);
    }

    bool TableDocument::nestedListInsert(u32_t row, u32_t column, const std::vector<NestedHop>& path, u32_t index)
    {
        Schema* schema = this->schemaAtPath(column, path);
        if (schema == nullptr || !schema->is(rose::SchemaType::List))
        {
            return false;
        }
        Schema* element = schema->getElementSchema();
        if (element == nullptr)
        {
            return false;
        }
        Value* list = this->ensureAtPath(row, column, path);
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

    bool TableDocument::nestedListRemove(u32_t row, u32_t column, const std::vector<NestedHop>& path, u32_t index)
    {
        Value* list = this->valueAtPath(row, column, path);
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

    String TableDocument::nestedText(u32_t row, u32_t column, const std::vector<NestedHop>& path) const
    {
        if (path.empty())
        {
            return this->cellText(row, column);
        }
        return this->textFromValue(this->valueAtPath(row, column, path), this->leafAtPath(column, path));
    }

    bool TableDocument::setNestedText(u32_t row, u32_t column, const std::vector<NestedHop>& path, const String& text)
    {
        if (!mOpen || path.empty() || row >= this->rowCount() || column >= this->columnCount())
        {
            return false;
        }
        String before = this->nestedText(row, column, path);
        NestedField leaf = this->leafAtPath(column, path);
        Value* parent = this->ensureParentAtPath(row, column, path);
        if (parent == nullptr)
        {
            return false;
        }
        this->writeFieldValue(parent, path.back(), leaf, trimCopy(text));
        if (before != this->nestedText(row, column, path))
        {
            this->markDirty();
        }
        return true;
    }

    std::vector<String> TableDocument::nestedLabels(u32_t row, u32_t column, const std::vector<NestedHop>& path) const
    {
        if (path.empty())
        {
            return this->cellLabels(row, column);
        }
        return this->labelsFromValue(this->valueAtPath(row, column, path), this->leafAtPath(column, path));
    }

    bool TableDocument::setNestedLabels(u32_t row, u32_t column, const std::vector<NestedHop>& path, const std::vector<String>& labels)
    {
        if (!mOpen || path.empty() || row >= this->rowCount() || column >= this->columnCount())
        {
            return false;
        }
        NestedField leaf = this->leafAtPath(column, path);
        Value* parent = this->ensureParentAtPath(row, column, path);
        if (parent == nullptr)
        {
            return false;
        }
        this->writeFieldLabels(parent, path.back(), leaf, labels);
        this->markDirty();
        return true;
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

    void TableDocument::writeFieldValue(rose::Value* parent, const NestedHop& hop, const NestedField& info, const String& text)
    {
        if (parent == nullptr || mLibrary == nullptr)
        {
            return;
        }
        if (info.kind == FieldKind::MultiOption)
        {
            this->writeFieldLabels(parent, hop, info, splitSelected(text));
            return;
        }
        Value* made = this->makeStoredValue(info.kind, info.schemaName, info.options, text, info.nestedStruct || info.nestedList);
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
        this->markDirty();
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
        this->markDirty();
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
        return this->displayFromValue(this->fieldAt(this->rowValue(row), info.id), this->objectField(column));
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
            this->markDirty();
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
        if (column.kind == FieldKind::MultiOption)
        {
            this->writeLabels(row, column, splitSelected(text));
            return;
        }
        Value* made = this->makeStoredValue(column.kind, column.schemaName, column.options, text, this->isNestedColumnType(column.schemaName));
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
            this->markDirty();
        }
        if (!valid)
        {
            this->setStatus(DocStatus::TypeMismatchCleared);
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
        this->markDirty();
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
        Schema* storage = mLibrary != nullptr ? mLibrary->getSchema(next) : nullptr;
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
        info.schemaName = storage->name();
        info.kind = kind;
        if (!mLibrary->set(this->columnValue(column), "kind", info.schemaName))
        {
            Value* fresh = this->makeColumn(info.id, info.name, info.schemaName);
            if (fresh == nullptr)
            {
                return false;
            }
            mLibrary->set(fresh, "width", static_cast<f64_t>(info.width));
            mLibrary->set(mColumns, column, fresh);
        }
        if (kind != FieldKind::SingleOption && kind != FieldKind::MultiOption)
        {
            info.options.clear();
        }
        else if (info.options.empty())
        {
            info.options = fieldOptionsFromSchema(optionsSchemaOf(storage));
        }
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
        this->markDirty();
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
        this->markDirty();
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
        this->markDirty();
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
        this->markDirty();
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
            this->markDirty();
            return;
        }
        TableColumn info = this->column(index);
        mRowSchema->delMember(info.id);
        this->rebuildRows(true);
        mLibrary->remove(mColumns, index);
        this->markDirty();
    }

}
