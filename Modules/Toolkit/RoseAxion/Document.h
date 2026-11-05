#ifndef _EOKAS_ROSE_AXION_DOCUMENT_H_
#define _EOKAS_ROSE_AXION_DOCUMENT_H_

#include "./FieldTypes.h"

#include "Rose/Library.h"

#include <memory>
#include <vector>

namespace eokas::RoseAxion {

    struct FieldOption
    {
        String label;
        String color;
    };

    struct TableColumn
    {
        String id;
        String name;
        FieldKind kind = FieldKind::Text;
        std::vector<FieldOption> options;
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

        u32_t rowCount() const;
        u32_t columnCount() const;
        TableColumn column(u32_t index) const;

        bool cellIsNull(u32_t row, u32_t column) const;
        String cellText(u32_t row, u32_t column) const;
        bool setCellText(u32_t row, u32_t column, const String& text);

        bool setColumnName(u32_t column, const String& name);
        bool columnChangeClears(u32_t column, FieldKind kind) const;
        bool setColumnType(u32_t column, FieldKind kind);
        bool setOptions(u32_t column, const std::vector<FieldOption>& options);

        void insertRow(u32_t index);
        void deleteRow(u32_t index);
        void insertColumn(u32_t index);
        void deleteColumn(u32_t index);

    private:
        bool bind();
        void refreshIds();
        rose::Schema* storage(FieldKind kind) const;
        rose::Value* columnValue(u32_t index) const;
        rose::Value* rowValue(u32_t index) const;
        String readString(rose::Value* object, const char* field) const;
        void writeCell(u32_t row, const TableColumn& column, const String& text);
        void putNull(rose::Value* row, const String& field, rose::Schema* schema);
        rose::Value* makeColumn(const String& id, const String& name, FieldKind kind, const std::vector<FieldOption>& options);
        rose::Value* makeOptions(const std::vector<FieldOption>& options);
        String makeId();
        rose::Schema* noneSchema();
        rose::Value* makeNone();
        rose::Value* fieldAt(rose::Value* object, const String& field) const;
        void writeOptions(rose::Value* list, const std::vector<FieldOption>& options);
        bool rebuildRows(bool replaceAll);

        std::unique_ptr<rose::Library> mLibrary;
        rose::Schema* mRowSchema = nullptr;
        rose::Schema* mString = nullptr;
        rose::Schema* mInt = nullptr;
        rose::Schema* mFloat = nullptr;
        rose::Schema* mBool = nullptr;
        rose::Schema* mOptionSchema = nullptr;
        rose::Schema* mOptionsSchema = nullptr;
        rose::Schema* mColumnSchema = nullptr;
        rose::Value* mColumns = nullptr;
        rose::Value* mRows = nullptr;
        String mPath;
        String mStatus;
        u32_t mNextId = 0;
        bool mOpen = false;
        bool mDirty = false;
    };

}

#endif//_EOKAS_ROSE_AXION_DOCUMENT_H_
