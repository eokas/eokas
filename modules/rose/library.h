
#ifndef _EOKAS_DATAPOT_LIBRARY_H_
#define _EOKAS_DATAPOT_LIBRARY_H_

#include "./header.h"
#include "./value.h"
#include "./schema.h"

namespace eokas::datapot {
    class Library {
    public:
        Library(const String& name);
        virtual ~Library();
        
        const String& name() const { return mName; }
        
        bool load(const String& filePath);
        bool save(const String& filePath);
        
        bool load(BinaryStream& stream);
        bool save(BinaryStream& stream);
        
        Schema* addSchema(SchemaType type, const String& name);
        Schema* getSchema(const String& name) const;
        Schema* getSchema(u32_t index) const;
        u32_t getSchemaCount() const;
        u32_t getSchemaIndex(const String& name) const;

        void getRootNames(std::vector<String>& names) const;

        Value* make(Schema* schema);
        Value* make(i32_t val);
        Value* make(i64_t val);
        Value* make(f64_t val);
        Value* make(bool val);
        Value* make(const String& val);

        bool get(Value* ptr, i32_t& val);
        bool get(Value* ptr, i64_t& val);
        bool get(Value* ptr, f64_t& val);
        bool get(Value* ptr, bool& val);
        bool get(Value* ptr, String& val);

        bool set(Value* ptr, i32_t val);
        bool set(Value* ptr, i64_t val);
        bool set(Value* ptr, f64_t val);
        bool set(Value* ptr, bool val);
        bool set(Value* ptr, const String& val);

        Value* get(const String& name);
        bool get(const String& name, i32_t& val);
        bool get(const String& name, i64_t& val);
        bool get(const String& name, f64_t& val);
        bool get(const String& name, bool& val);
        bool get(const String& name, String& val);

        void set(const String& name, Value* val);
        void set(const String& name, i32_t val);
        void set(const String& name, i64_t val);
        void set(const String& name, f64_t val);
        void set(const String& name, bool val);
        void set(const String& name, const String& val);
        
        
        Value* get(Value* list, u32_t index);
        bool get(Value* list, u32_t index, i32_t& val);
        bool get(Value* list, u32_t index, i64_t& val);
        bool get(Value* list, u32_t index, f64_t& val);
        bool get(Value* list, u32_t index, bool& val);
        bool get(Value* list, u32_t index, String& val);

        bool set(Value* list, u32_t index, Value* val);
        bool set(Value* list, u32_t index, i32_t val);
        bool set(Value* list, u32_t index, i64_t val);
        bool set(Value* list, u32_t index, f64_t val);
        bool set(Value* list, u32_t index, bool val);
        bool set(Value* list, u32_t index, const String& val);

        bool push(Value* list, Value* val);
        bool push(Value* list, i32_t val);
        bool push(Value* list, i64_t val);
        bool push(Value* list, f64_t val);
        bool push(Value* list, bool val);
        bool push(Value* list, const String& val);

        bool pop(Value* list, Value* val);
        bool pop(Value* list, i32_t& val);
        bool pop(Value* list, i64_t& val);
        bool pop(Value* list, f64_t& val);
        bool pop(Value* list, bool& val);
        bool pop(Value* list, String& val);

        Value* get(Value* object, const String& field);
        bool get(Value* object, const String& field, i32_t& val);
        bool get(Value* object, const String& field, i64_t& val);
        bool get(Value* object, const String& field, f64_t& val);
        bool get(Value* object, const String& field, bool& val);
        bool get(Value* object, const String& field, String& val);

        bool set(Value* object, const String& field, Value* val);
        bool set(Value* object, const String& field, i32_t val);
        bool set(Value* object, const String& field, i64_t val);
        bool set(Value* object, const String& field, f64_t val);
        bool set(Value* object, const String& field, bool val);
        bool set(Value* object, const String& field, const String& val);
        
    private:
        String mName;

        SchemaHeap mSchemas;
        ValueHeap mValues;

        std::map<String, Value*> mRoot;
    };
}

#endif//_EOKAS_DATAPOT_LIBRARY_H_
