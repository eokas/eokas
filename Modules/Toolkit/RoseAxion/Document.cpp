#include "./Document.h"

#include <stdexcept>

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

    }

    void TableDocument::createNew()
    {
        mLibrary = std::make_unique<rose::Library>("table");
        mString = mLibrary->getSchema("String");
        mInt = mLibrary->getSchema("Int");
        mFloat = mLibrary->getSchema("Float");
        mBool = mLibrary->getSchema("Bool");
        this->ensureSelectedSchema();

        mOptionSchema = mLibrary->addSchema(rose::SchemaType::Struct, "Option");
        mOptionSchema->addMember("label", mString);
        mOptionSchema->addMember("color", mString);
        mOptionsSchema = mLibrary->addSchema(rose::SchemaType::List, "Options");
        mOptionsSchema->setElementSchema(mOptionSchema);

        mColumnSchema = mLibrary->addSchema(rose::SchemaType::Struct, "Column");
        mColumnSchema->addMember("id", mString);
        mColumnSchema->addMember("name", mString);
        mColumnSchema->addMember("kind", mString);
        mColumnSchema->addMember("options", mOptionsSchema);
        mColumnSchema->addMember("width", mFloat);
        rose::Schema* columns = mLibrary->addSchema(rose::SchemaType::List, "Columns");
        columns->setElementSchema(mColumnSchema);

        mRowSchema = mLibrary->addSchema(rose::SchemaType::Struct, "Row");
        rose::Schema* rows = mLibrary->addSchema(rose::SchemaType::List, "Rows");
        rows->setElementSchema(mRowSchema);

        mColumns = mLibrary->make(columns);
        mRows = mLibrary->make(rows);
        mLibrary->set("columns", mColumns);
        mLibrary->set("rows", mRows);
        mLibrary->set("headWidth", static_cast<f64_t>(kDefaultHeadWidth));
        mNextId = 0;
        mPath = String::empty;
        mOpen = true;
        mDirty = false;
        this->insertColumn(0);
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
        bool wasOpen = mOpen;
        rose::Schema* previousRow = mRowSchema;
        rose::Schema* previousString = mString;
        rose::Schema* previousInt = mInt;
        rose::Schema* previousFloat = mFloat;
        rose::Schema* previousBool = mBool;
        rose::Schema* previousOption = mOptionSchema;
        rose::Schema* previousOptions = mOptionsSchema;
        rose::Schema* previousSelected = mSelectedSchema;
        rose::Schema* previousColumn = mColumnSchema;
        rose::Value* previousColumns = mColumns;
        rose::Value* previousRows = mRows;
        String previousPath = mPath;
        u32_t previousNext = mNextId;

        mLibrary = std::move(next);
        if (!this->bind())
        {
            mLibrary = std::move(previous);
            mOpen = wasOpen;
            mRowSchema = previousRow;
            mString = previousString;
            mInt = previousInt;
            mFloat = previousFloat;
            mBool = previousBool;
            mOptionSchema = previousOption;
            mOptionsSchema = previousOptions;
            mSelectedSchema = previousSelected;
            mColumnSchema = previousColumn;
            mColumns = previousColumns;
            mRows = previousRows;
            mPath = previousPath;
            mNextId = previousNext;
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
        if (mLibrary == nullptr)
        {
            return false;
        }
        mString = mLibrary->getSchema("String");
        mInt = mLibrary->getSchema("Int");
        mFloat = mLibrary->getSchema("Float");
        mBool = mLibrary->getSchema("Bool");
        mRowSchema = mLibrary->getSchema("Row");
        mOptionSchema = mLibrary->getSchema("Option");
        mOptionsSchema = mLibrary->getSchema("Options");
        this->ensureSelectedSchema();
        mColumnSchema = mLibrary->getSchema("Column");
        mColumns = mLibrary->get("columns");
        mRows = mLibrary->get("rows");
        if (mString == nullptr || mInt == nullptr || mFloat == nullptr || mBool == nullptr)
        {
            return false;
        }
        if (mRowSchema == nullptr || mOptionSchema == nullptr || mOptionsSchema == nullptr || mColumnSchema == nullptr || mSelectedSchema == nullptr)
        {
            return false;
        }
        if (mColumns == nullptr || mRows == nullptr)
        {
            return false;
        }
        if (!mColumns->schema->is(rose::SchemaType::List) || !mRows->schema->is(rose::SchemaType::List))
        {
            return false;
        }
        this->refreshIds();
        this->ensureLayoutFields();
        mOpen = true;
        mDirty = this->rebuildRows(false);
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

    rose::Schema* TableDocument::storage(FieldKind kind) const
    {
        switch (kind)
        {
        case FieldKind::Integer: return mInt;
        case FieldKind::Float: return mFloat;
        case FieldKind::Boolean: return mBool;
        case FieldKind::MultiOption: return mSelectedSchema;
        default: return mString;
        }
    }

    u32_t TableDocument::rowCount() const
    {
        if (mLibrary == nullptr)
        {
            return 0;
        }
        return mLibrary->count(mRows);
    }

    u32_t TableDocument::columnCount() const
    {
        if (mLibrary == nullptr)
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
        column.kind = fieldKindFromKey(this->readString(object, "kind"));
        Value* options = this->fieldAt(object, "options");
        u32_t count = mLibrary->count(options);
        for (u32_t optionIndex = 0; optionIndex < count; ++optionIndex)
        {
            Value* option = mLibrary->get(options, optionIndex);
            FieldOption item;
            item.label = this->readString(option, "label");
            item.color = this->readString(option, "color");
            column.options.push_back(item);
        }
        column.width = this->readFloat(object, "width", kDefaultColumnWidth);
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
        Value* fresh = this->makeColumn(info.id, info.name, info.kind, info.options);
        mLibrary->set(fresh, "width", static_cast<f64_t>(clampWidth(width)));
        mLibrary->set(mColumns, index, fresh);
        return fresh;
    }

    void TableDocument::ensureLayoutFields()
    {
        if (mColumnSchema != nullptr && mColumnSchema->getMember("width") == nullptr)
        {
            mColumnSchema->addMember("width", mFloat);
        }
        u32_t count = this->columnCount();
        for (u32_t index = 0; index < count; ++index)
        {
            if (this->fieldAt(this->columnValue(index), "width") == nullptr)
            {
                this->rebuildColumn(index, kDefaultColumnWidth);
            }
        }
        f64_t head = 0.0;
        if (!mLibrary->get("headWidth", head))
        {
            mLibrary->set("headWidth", static_cast<f64_t>(kDefaultHeadWidth));
        }
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
        if (!mOpen || mLibrary == nullptr)
        {
            return kDefaultHeadWidth;
        }
        f64_t value = kDefaultHeadWidth;
        if (!mLibrary->get("headWidth", value))
        {
            return kDefaultHeadWidth;
        }
        return clampWidth(static_cast<float>(value));
    }

    bool TableDocument::setHeadWidth(float width)
    {
        if (!mOpen || mLibrary == nullptr)
        {
            return false;
        }
        float next = clampWidth(width);
        if (this->headWidth() == next)
        {
            return true;
        }
        mLibrary->set("headWidth", static_cast<f64_t>(next));
        mDirty = true;
        return true;
    }

    void TableDocument::ensureSelectedSchema()
    {
        mSelectedSchema = mLibrary->getSchema("Selected");
        if (mSelectedSchema == nullptr)
        {
            mSelectedSchema = mLibrary->addSchema(rose::SchemaType::List, "Selected");
        }
        if (mSelectedSchema != nullptr)
        {
            mSelectedSchema->setElementSchema(mString);
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

    std::vector<String> TableDocument::readLabels(u32_t row, u32_t column) const
    {
        std::vector<String> labels;
        if (!mOpen || row >= this->rowCount() || column >= this->columnCount())
        {
            return labels;
        }
        TableColumn info = this->column(column);
        Value* field = this->fieldAt(this->rowValue(row), info.id);
        if (field == nullptr || field->schema == nullptr || field->schema->is(rose::SchemaType::None))
        {
            return labels;
        }
        if (field->schema->is(rose::SchemaType::List))
        {
            u32_t count = mLibrary->count(field);
            for (u32_t index = 0; index < count; ++index)
            {
                String item;
                if (mLibrary->get(field, index, item) && !item.isEmpty())
                {
                    labels.push_back(item);
                }
            }
            return labels;
        }
        String value;
        if (mLibrary->get(field, value) && !value.isEmpty())
        {
            return splitSelected(value);
        }
        return labels;
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
            made = mLibrary->make(mSelectedSchema);
            if (made != nullptr)
            {
                mLibrary->clear(made);
                for (const String& label : filtered)
                {
                    mLibrary->push(made, label);
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
        if (info.kind == FieldKind::DateTime)
        {
            return displayDateTime(value);
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
        return this->filterLabels(this->readLabels(row, column), info.options);
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

    bool TableDocument::renameOptionLabel(u32_t column, const String& from, const String& to)
    {
        if (!mOpen || column >= this->columnCount())
        {
            return false;
        }
        if (from == to || from.isEmpty())
        {
            return true;
        }
        TableColumn info = this->column(column);
        u32_t rows = this->rowCount();
        for (u32_t row = 0; row < rows; ++row)
        {
            if (info.kind == FieldKind::MultiOption)
            {
                std::vector<String> labels = this->readLabels(row, column);
                bool changed = false;
                for (String& item : labels)
                {
                    if (item == from)
                    {
                        item = to;
                        changed = true;
                    }
                }
                if (changed)
                {
                    this->setCellLabels(row, column, labels);
                }
            }
            else if (this->cellText(row, column) == from)
            {
                this->setCellText(row, column, to);
            }
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
        u32_t rows = this->rowCount();
        for (u32_t row = 0; row < rows; ++row)
        {
            if (info.kind == FieldKind::MultiOption)
            {
                std::vector<String> labels = this->readLabels(row, column);
                std::vector<String> kept;
                bool changed = false;
                for (const String& item : labels)
                {
                    if (item == label)
                    {
                        changed = true;
                        continue;
                    }
                    kept.push_back(item);
                }
                if (changed)
                {
                    this->setCellLabels(row, column, kept);
                }
            }
            else if (this->cellText(row, column) == label)
            {
                this->setCellText(row, column, String::empty);
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
        String value = trimCopy(text);
        Schema* schema = this->storage(column.kind);
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

    bool TableDocument::columnChangeClears(u32_t column, FieldKind kind) const
    {
        if (!mOpen || column >= this->columnCount())
        {
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

    bool TableDocument::setColumnType(u32_t column, FieldKind kind)
    {
        if (!mOpen || column >= this->columnCount())
        {
            return false;
        }
        TableColumn info = this->column(column);
        if (info.kind == kind)
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
        mRowSchema->setMember(info.id, this->storage(kind));
        mLibrary->set(this->columnValue(column), "kind", String(fieldKindKey(kind)));
        if (kind != FieldKind::SingleOption && kind != FieldKind::MultiOption)
        {
            this->setOptions(column, {});
            info.options.clear();
        }
        else if (info.options.empty())
        {
            this->setOptions(column, {});
        }
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

    void TableDocument::writeOptions(rose::Value* list, const std::vector<FieldOption>& options)
    {
        if (list == nullptr)
        {
            return;
        }
        Value listValue = *list;
        mLibrary->clear(&listValue);
        for (const FieldOption& option : options)
        {
            Value* item = mLibrary->make(mOptionSchema);
            mLibrary->set(item, "label", option.label);
            mLibrary->set(item, "color", option.color.isEmpty() ? String("#6366f1") : option.color);
            mLibrary->push(&listValue, item);
        }
    }

    rose::Value* TableDocument::makeOptions(const std::vector<FieldOption>& options)
    {
        Value* list = mLibrary->make(mOptionsSchema);
        this->writeOptions(list, options);
        return list;
    }

    bool TableDocument::setOptions(u32_t column, const std::vector<FieldOption>& options)
    {
        if (!mOpen || column >= this->columnCount())
        {
            return false;
        }
        Value* columnValue = this->columnValue(column);
        Value* list = this->fieldAt(columnValue, "options");
        if (list == nullptr)
        {
            mLibrary->set(columnValue, "options", this->makeOptions(options));
        }
        else
        {
            this->writeOptions(list, options);
        }
        mDirty = true;
        return true;
    }

    String TableDocument::makeId()
    {
        String id = String::format("c%u", mNextId);
        ++mNextId;
        return id;
    }

    rose::Value* TableDocument::makeColumn(const String& id, const String& name, FieldKind kind, const std::vector<FieldOption>& options)
    {
        Value* column = mLibrary->make(mColumnSchema);
        mLibrary->set(column, "id", id);
        mLibrary->set(column, "name", name);
        mLibrary->set(column, "kind", String(fieldKindKey(kind)));
        mLibrary->set(column, "options", this->makeOptions(options));
        mLibrary->set(column, "width", static_cast<f64_t>(kDefaultColumnWidth));
        return column;
    }

    void TableDocument::insertRow(u32_t index)
    {
        if (!mOpen)
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
            this->putNull(row, info.id, this->storage(info.kind));
        }
        if (!mLibrary->insert(mRows, index, row))
        {
            return;
        }
        mDirty = true;
    }

    void TableDocument::deleteRow(u32_t index)
    {
        if (!mOpen || index >= this->rowCount())
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
        if (!mOpen || mRowSchema == nullptr)
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
        mRowSchema->addMember(id, mString);
        this->rebuildRows(false);
        Value* column = this->makeColumn(id, name, FieldKind::Text, {});
        mLibrary->insert(mColumns, index, column);
        if (this->rowCount() == 0)
        {
            this->insertRow(0);
        }
        mDirty = true;
    }

    void TableDocument::deleteColumn(u32_t index)
    {
        if (!mOpen || index >= this->columnCount())
        {
            return;
        }
        if (this->columnCount() <= 1)
        {
            TableColumn info = this->column(index);
            mRowSchema->setMember(info.id, mString);
            mLibrary->set(this->columnValue(index), "kind", String(fieldKindKey(FieldKind::Text)));
            this->setOptions(index, {});
            u32_t rows = this->rowCount();
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
