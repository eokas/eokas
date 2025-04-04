#ifndef _EOKAS_DATAPOT_VALUE_H_
#define _EOKAS_DATAPOT_VALUE_H_

#include "./header.h"

namespace eokas::datapot {
    struct Value {
        Schema* schema;
        union {
            i64_t i64;
            u64_t u64;
            f64_t f64;
        } value;
        
        void set(Schema* schema, i32_t val) {
            this->schema = schema;
            this->value.i64 = val;
        }
        
        void set(Schema* schema, u32_t val) {
            this->schema = schema;
            this->value.u64 = val;
        }
        
        void set(Schema* schema, i64_t val) {
            this->schema = schema;
            this->value.i64 = val;
        }
        
        void set(Schema* schema, u64_t val) {
            this->schema = schema;
            this->value.u64 = val;
        }

        void set(Schema* schema, f64_t val) {
            this->schema = schema;
            this->value.f64 = val;
        }
    };
    
    struct List {
        std::vector<Value> elements;

        Value& get(u32_t index) {
            return elements.at(index);
        }

        void set(u32_t index, const Value& value) {
            elements.at(index) = value;
        }

        void push(const Value& value) {
            elements.push_back(value);
        }

        bool pop(Value& value) {
            if(elements.empty()) {
                return false;
            }
            value = elements.back();
            elements.pop_back();
            return true;
        }
    };
    
    struct Object {
        std::map<String, Value> members;

        Value& get(const String& name) {
            return members.at(name);
        }

        void set(const String& name, const Value& value) {
            members.at(name) = value;
        }
    };
    
    class ValueHeap {
    public:
        ValueHeap(SchemaHeap& schemaHeap);
        virtual ~ValueHeap();

        std::vector<Value> values;
        
        std::vector<List> lists;
        std::vector<Object> objects;
        
        std::vector<String> strings;
        
        u32_t indexOf(Value* val);
        
        Value* make(i32_t val);
        Value* make(i64_t val);
        Value* make(f64_t val);
        Value* make(bool val);
        Value* make(const String& val);
        Value* make(Schema* schema);

        Value* get(Value* list, u32_t index);
        bool set(Value* list, u32_t index, Value* val);
        bool push(Value* list, Value* val);
        bool pop(Value* list, Value* val);

        Value* get(Value* object, const String& field);
        bool set(Value* object, const String& field, Value* val);

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

        void clear();

    private:
        SchemaHeap& mSchemaHeap;
    };
}

#endif//_EOKAS_DATAPOT_VALUE_H_