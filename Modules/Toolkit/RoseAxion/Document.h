#ifndef _EOKAS_ROSE_AXION_DOCUMENT_H_
#define _EOKAS_ROSE_AXION_DOCUMENT_H_

#include "./FieldTypes.h"

#include "Rose/Library.h"

#include <memory>
#include <vector>

namespace eokas::RoseAxion {

    struct FieldOption
    {
        String name;
        String label;
        String color;
    };

    struct TableColumn
    {
        String id;
        String name;
        String schemaName = "String";
        FieldKind kind = FieldKind::Text;
        std::vector<FieldOption> options;
        float width = 168.0f;
    };

    class TableDocument
    {
    public:
        bool isOpen() const { return mOpen; }
        bool dirty() const { return mDirty; }
        const String& path() const { return mPath; }
        const String& status() const { return mStatus; }
        void setStatus(const String& text) { mStatus = text; }

        void createNew();
        bool load(const String& path);
        bool save(const String& path);

        u32_t tableCount() const;
        bool hasActiveTable() const;
        u32_t activeTableIndex() const { return mActiveIndex < 0 ? 0 : static_cast<u32_t>(mActiveIndex); }
        String tableId(u32_t index) const;
        String tableName(u32_t index) const;
        bool selectTable(u32_t index);
        bool addTable();
        bool renameTable(u32_t index, const String& name);
        bool deleteTable(u32_t index);

        u32_t userSchemaCount() const;
        bool hasActiveSchema() const;
        u32_t activeSchemaIndex() const { return mActiveSchema < 0 ? 0 : static_cast<u32_t>(mActiveSchema); }
        bool selectSchema(u32_t index);
        String schemaName(u32_t index) const;
        rose::SchemaType schemaType(u32_t index) const;
        bool addUserSchema(rose::SchemaType type);
        bool renameUserSchema(u32_t index, const String& name);
        bool deleteUserSchema(u32_t index);

        u32_t schemaMemberCount() const;
        String schemaMemberName(u32_t memberIndex) const;
        String schemaMemberTypeName(u32_t memberIndex) const;
        bool addSchemaMember();
        bool renameSchemaMember(u32_t memberIndex, const String& name);
        bool setSchemaMemberType(u32_t memberIndex, const String& typeName);
        bool deleteSchemaMember(u32_t memberIndex);

        String schemaElementTypeName() const;
        bool setSchemaElementType(const String& typeName);
        std::vector<String> schemaAssignableTypeNames() const;
        std::vector<String> columnTypeNames() const;
        u32_t schemaOptionCount() const;
        String schemaOptionLabel(u32_t index) const;
        bool setSchemaOptionLabel(u32_t index, const String& label);
        bool addSchemaOption();
        bool deleteSchemaOption(u32_t index);

        bool isNestedColumn(u32_t column) const;

        u32_t rowCount() const;
        u32_t columnCount() const;
        TableColumn column(u32_t index) const;
        float columnWidth(u32_t index) const;
        bool setColumnWidth(u32_t index, float width);
        float headWidth() const;
        bool setHeadWidth(float width);

        bool cellIsNull(u32_t row, u32_t column) const;
        String cellText(u32_t row, u32_t column) const;
        bool setCellText(u32_t row, u32_t column, const String& text);
        std::vector<String> cellLabels(u32_t row, u32_t column) const;
        bool setCellLabels(u32_t row, u32_t column, const std::vector<String>& labels);
        bool dropOptionLabel(u32_t column, const String& label);

        bool setColumnName(u32_t column, const String& name);
        bool columnChangeClears(u32_t column, const String& typeName) const;
        bool setColumnType(u32_t column, const String& typeName);
        bool setOptions(u32_t column, const std::vector<FieldOption>& options);

        void insertRow(u32_t index);
        void deleteRow(u32_t index);
        void insertColumn(u32_t index);
        void deleteColumn(u32_t index);

    private:
        bool bind();
        bool ensureSharedSchemas(bool install);
        bool bindTable(rose::Value* table, int index);
        void clearActive();
        void refreshIds();
        void refreshTableIds();
        rose::Value* tableValue(u32_t index) const;
        String nextNewTableName() const;
        String makeTableId();
        bool tableNameTaken(const String& name, int skipIndex) const;
        bool isReservedSchemaName(const String& name) const;
        bool isScaffoldingSchemaName(const String& name) const;
        bool isUserSchemaName(const String& name) const;
        rose::Schema* userSchemaAt(u32_t index) const;
        rose::Schema* activeSchema() const;
        String nextNewSchemaName() const;
        bool schemaNameTaken(const String& name, int skipIndex) const;
        bool schemaReaches(rose::Schema* start, rose::Schema* target) const;
        bool wouldCycle(rose::Schema* from, rose::Schema* to) const;
        bool schemaReferenced(rose::Schema* schema) const;
        String nextMemberName(rose::Schema* schema) const;
        rose::Schema* resolveAssignableType(const String& typeName, rose::Schema* current) const;
        float readFloat(rose::Value* object, const char* field, float fallback) const;
        rose::Value* rebuildColumn(u32_t index, float width);
        FieldKind fieldKindForColumnType(const String& typeName) const;
        rose::Schema* storageForType(const String& typeName) const;
        rose::Schema* optionsSchemaForStorage(rose::Schema* storage) const;
        bool isNestedColumnType(const String& typeName) const;
        rose::Value* columnValue(u32_t index) const;
        rose::Value* rowValue(u32_t index) const;
        String readString(rose::Value* object, const char* field) const;
        void writeCell(u32_t row, const TableColumn& column, const String& text);
        void putNull(rose::Value* row, const String& field, rose::Schema* schema);
        std::vector<String> readStoredOptionNames(u32_t row, u32_t column) const;
        std::vector<String> filterLabels(const std::vector<String>& labels, const std::vector<FieldOption>& options) const;
        void writeLabels(u32_t row, const TableColumn& column, const std::vector<String>& labels);
        rose::Value* makeColumn(const String& id, const String& name, const String& schemaName);
        String makeId();
        rose::Schema* noneSchema();
        rose::Value* makeNone();
        rose::Value* fieldAt(rose::Value* object, const String& field) const;
        bool rebuildRows(bool replaceAll);

        std::unique_ptr<rose::Library> mLibrary;
        rose::Schema* mRowSchema = nullptr;
        rose::Schema* mString = nullptr;
        rose::Schema* mInt = nullptr;
        rose::Schema* mFloat = nullptr;
        rose::Schema* mBool = nullptr;
        rose::Schema* mColumnSchema = nullptr;
        rose::Schema* mColumnsSchema = nullptr;
        rose::Schema* mTableSchema = nullptr;
        rose::Schema* mTablesSchema = nullptr;
        rose::Value* mTables = nullptr;
        rose::Value* mActiveTable = nullptr;
        rose::Value* mColumns = nullptr;
        rose::Value* mRows = nullptr;
        String mPath;
        String mStatus;
        u32_t mNextId = 0;
        u32_t mNextTableId = 0;
        int mActiveIndex = -1;
        int mActiveSchema = -1;
        bool mOpen = false;
        bool mDirty = false;
    };

}

#endif//_EOKAS_ROSE_AXION_DOCUMENT_H_
