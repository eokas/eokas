#pragma once
#include "./Header.h"

#include <deque>

namespace eokas::rose {
    struct Value
    {
        Schema* schema;
        union
        {
            i64_t i64;
            u64_t u64;
            f64_t f64;
        } value;
        
        void set(Schema* schema, i32_t val)
        {
            this->schema = schema;
            this->value.i64 = val;
        }
        
        void set(Schema* schema, u32_t val)
        {
            this->schema = schema;
            this->value.u64 = val;
        }
        
        void set(Schema* schema, i64_t val)
        {
            this->schema = schema;
            this->value.i64 = val;
        }
        
        void set(Schema* schema, u64_t val)
        {
            this->schema = schema;
            this->value.u64 = val;
        }

        void set(Schema* schema, f64_t val)
        {
            this->schema = schema;
            this->value.f64 = val;
        }
    };
    
    struct List
    {
        std::vector<Value> elements;

        u32_t count() const
        {
            return u32_t(elements.size());
        }

        Value& get(u32_t index)
        {
            return elements.at(index);
        }

        void set(u32_t index, const Value& value)
        {
            elements.at(index) = value;
        }

        bool insert(u32_t index, const Value& value)
        {
            if(index > elements.size())
                return false;
            elements.insert(elements.begin() + index, value);
            return true;
        }

        bool remove(u32_t index)
        {
            if(index >= elements.size())
                return false;
            elements.erase(elements.begin() + index);
            return true;
        }

        void push(const Value& value)
        {
            elements.push_back(value);
        }

        bool pop(Value& value)
        {
            if(elements.empty())
            {
                return false;
            }
            value = elements.back();
            elements.pop_back();
            return true;
        }

        void clear()
        {
            elements.clear();
        }
    };
    
    struct Object
    {
        std::map<String, Value> members;

        Value& get(const String& name)
        {
            return members.at(name);
        }

        bool set(const String& name, const Value& value)
        {
            auto found = members.find(name);
            if (found == members.end())
                return false;
            found->second = value;
            return true;
        }
    };
    
    class ValueHeap
    {
    public:
        ValueHeap(SchemaHeap& schemaHeap);
        virtual ~ValueHeap();

        std::deque<Value> values;
        
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

        u32_t count(Value* list);
        Value* get(Value* list, u32_t index);
        bool set(Value* list, u32_t index, Value* val);
        bool insert(Value* list, u32_t index, Value* val);
        bool remove(Value* list, u32_t index);
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

        bool clear(Value* value);
        void clear();

    private:
        SchemaHeap& mSchemaHeap;
    };
}
