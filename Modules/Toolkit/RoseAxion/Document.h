#ifndef _EOKAS_ROSE_AXION_DOCUMENT_H_
#define _EOKAS_ROSE_AXION_DOCUMENT_H_

#include "./FieldTypes.h"

#include "Rose/Library.h"

#include <memory>
#include <vector>

namespace eokas::RoseAxion {

    enum class DocStatus
    {
        Ready,
        Opened,
        Saved,
        OpenFailed,
        CannotOpenFile,
        InvalidLibrary,
        SaveFailed,
        CannotSaveFile,
        SchemaNameEmpty,
        SchemaNameTaken,
        CannotCreateSchema,
        CannotDeleteSchema,
        CannotAddMember,
        MemberNameEmpty,
        MemberNameTaken,
        OptionNameEmpty,
        OptionNameTaken,
        CannotUpdateOptions,
        InstanceNameEmpty,
        InstanceNameTaken,
        CannotCreateInstance,
        CannotDeleteInstance,
        TypeMismatchCleared,
    };

    const char* docStatusText(DocStatus status);
    bool docStatusFailed(DocStatus status);

    struct NestedHop
    {
        bool list = false;
        String member;
        u32_t index = 0;
        static NestedHop field(const String& name)
        {
            NestedHop hop;
            hop.member = name;
            return hop;
        }
        static NestedHop at(u32_t index)
        {
            NestedHop hop;
            hop.list = true;
            hop.index = index;
            return hop;
        }
    };

    struct NestedField
    {
        String id;
        String name;
        String schemaName = "String";
        FieldKind kind = FieldKind::Text;
        std::vector<FieldOption> options;
        bool nestedStruct = false;
        bool nestedList = false;
        bool listItem = false;
    };

    class TableDocument
    {
    public:
        bool isOpen() const { return mOpen; }
        bool dirty() const { return mDirty; }
        const String& path() const { return mPath; }
        DocStatus statusKind() const { return mStatus; }
        String status() const { return docStatusText(mStatus); }
        void setStatus(DocStatus status) { mStatus = status; }

        void createNew();
        bool load(const String& path);
        bool save(const String& path);

        size_t loadedLibraryBytes() const { return mLoadedLibraryBytes; }
        String librarySizeText() const;

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
        bool moveSchemaMember(u32_t memberIndex, int delta);
        bool deleteSchemaMember(u32_t memberIndex);

        String schemaElementTypeName() const;
        bool setSchemaElementType(const String& typeName);
        std::vector<String> schemaAssignableTypeNames() const;
        std::vector<String> columnTypeNames() const;
        std::vector<FieldOption> schemaOptions() const;
        bool setSchemaOptions(const std::vector<FieldOption>& options);

        u32_t instanceCount() const;
        bool hasActiveInstance() const;
        u32_t activeInstanceIndex() const;
        bool selectInstance(u32_t index);
        String instanceName(u32_t index) const;
        String instanceSchemaName(u32_t index) const;
        rose::Value* instanceValue(u32_t index) const;
        rose::Value* activeInstanceValue() const;
        bool addInstance(const String& schemaName);
        bool renameInstance(u32_t index, const String& name);
        bool deleteInstance(u32_t index);

        NestedField instanceRootField() const;
        String instanceText() const;
        bool setInstanceText(const String& text);
        std::vector<String> instanceLabels() const;
        bool setInstanceLabels(const std::vector<String>& labels);

        u32_t instanceListCount() const;
        bool instanceListInsert(u32_t index);
        bool instanceListRemove(u32_t index);
        u32_t instanceListColumnCount() const;
        NestedField instanceListColumn(u32_t col) const;
        String instanceListCellText(u32_t row, u32_t col) const;
        bool setInstanceListCellText(u32_t row, u32_t col, const String& text);
        std::vector<String> instanceListCellLabels(u32_t row, u32_t col) const;
        bool setInstanceListCellLabels(u32_t row, u32_t col, const std::vector<String>& labels);

        u32_t instanceStructMemberCount() const;
        NestedField instanceStructMember(u32_t memberIndex) const;
        u32_t instanceNestedFieldCount(const std::vector<NestedHop>& path) const;
        NestedField instanceNestedField(const std::vector<NestedHop>& path, u32_t index) const;
        NestedField instanceNestedListElement(const std::vector<NestedHop>& path) const;
        u32_t instanceNestedListCount(const std::vector<NestedHop>& path) const;
        String instanceStructText(const std::vector<NestedHop>& path) const;
        bool setInstanceStructText(const std::vector<NestedHop>& path, const String& text);
        std::vector<String> instanceStructLabels(const std::vector<NestedHop>& path) const;
        bool setInstanceStructLabels(const std::vector<NestedHop>& path, const std::vector<String>& labels);
        bool instanceStructListInsert(const std::vector<NestedHop>& path, u32_t index);
        bool instanceStructListRemove(const std::vector<NestedHop>& path, u32_t index);

    private:
        bool bind();
        bool ensureSharedSchemas(bool install);
        void clearActive();
        rose::Schema* userSchemaAt(u32_t index) const;
        rose::Schema* activeSchema() const;
        String nextNewSchemaName() const;
        bool schemaNameTaken(const String& name, int skipIndex) const;
        bool schemaReaches(rose::Schema* start, rose::Schema* target) const;
        bool wouldCycle(rose::Schema* from, rose::Schema* to) const;
        bool schemaReferenced(rose::Schema* schema) const;
        String nextMemberName(rose::Schema* schema) const;
        rose::Schema* resolveAssignableType(const String& typeName, rose::Schema* current) const;
        FieldKind fieldKindForColumnType(const String& typeName) const;
        rose::Schema* storageForType(const String& typeName) const;
        bool isNestedColumnType(const String& typeName) const;
        bool isListType(const String& typeName) const;
        NestedField fieldFromStorage(rose::Schema* storage, const String& id, const String& name) const;
        void markDirty();
        String displayFromValue(rose::Value* field, const NestedField& info) const;
        String textFromValue(rose::Value* field, const NestedField& info) const;
        std::vector<String> labelsFromValue(rose::Value* field, const NestedField& info) const;
        rose::Value* makeStoredValue(FieldKind kind, const String& schemaName, const std::vector<FieldOption>& options, const String& text, bool nested);
        bool writeFieldValue(rose::Value* parent, const NestedHop& hop, const NestedField& info, const String& text);
        void writeFieldLabels(rose::Value* parent, const NestedHop& hop, const NestedField& info, const std::vector<String>& labels);
        std::vector<String> filterLabels(const std::vector<String>& labels, const std::vector<FieldOption>& options) const;
        rose::Schema* noneSchema();
        rose::Value* makeNone();
        rose::Value* fieldAt(rose::Value* object, const String& field) const;
        bool isReservedSchemaName(const String& name) const;
        bool isScaffoldingSchemaName(const String& name) const;
        bool isUserSchemaName(const String& name) const;

        bool ensureInstancesRegistry();
        bool isReservedRootName(const String& name) const;
        String nextNewInstanceName() const;
        bool instanceNameTaken(const String& name, int skipIndex) const;
        String instanceKeyAt(u32_t index) const;
        rose::Schema* instanceSchemaAtPath(const std::vector<NestedHop>& path) const;
        NestedField instanceLeafAtPath(const std::vector<NestedHop>& path) const;
        rose::Value* instanceValueAtPath(const std::vector<NestedHop>& path) const;
        rose::Value* instanceEnsureAtPath(const std::vector<NestedHop>& path);
        rose::Value* instanceEnsureParentAtPath(const std::vector<NestedHop>& path);

        std::unique_ptr<rose::Library> mLibrary;
        rose::Schema* mString = nullptr;
        rose::Schema* mInt = nullptr;
        rose::Schema* mFloat = nullptr;
        rose::Schema* mBool = nullptr;
        rose::Value* mInstances = nullptr;
        String mPath;
        DocStatus mStatus = DocStatus::Ready;
        int mActiveSchema = -1;
        int mActiveInstance = -1;
        bool mOpen = false;
        bool mDirty = false;
        size_t mLoadedLibraryBytes = 0;
    };

}

#endif