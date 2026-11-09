#include "./Value.h"
#include "./Schema.h"

namespace eokas::rose
{
    ValueHeap::ValueHeap(SchemaHeap &schemaHeap)
        : mSchemaHeap(schemaHeap) {}

    ValueHeap::~ValueHeap()
    {
        this->clear();
    }

    u32_t ValueHeap::indexOf(Value *val)
    {
        for (size_t index = 0; index < values.size(); index++)
        {
            if (val == &values.at(index))
            {
                return u32_t(index);
            }
        }
        return -1;
    }

    Value *ValueHeap::make(i32_t val)
    {
        Schema *schema = mSchemaHeap.get("Int");
        Value &value = this->values.emplace_back();
        value.set(schema, val);
        return &value;
    }

    Value *ValueHeap::make(i64_t val)
    {
        Schema *schema = mSchemaHeap.get("Int");
        Value &value = this->values.emplace_back();
        value.set(schema, val);
        return &value;
    }

    Value *ValueHeap::make(f64_t val)
    {
        Schema *schema = mSchemaHeap.get("Float");
        Value &value = this->values.emplace_back();
        value.set(schema, val);
        return &value;
    }

    Value *ValueHeap::make(bool val)
    {
        Schema *schema = mSchemaHeap.get("Bool");
        Value &value = this->values.emplace_back();
        value.set(schema, val ? 1 : 0);
        return &value;
    }

    Value *ValueHeap::make(const String &val)
    {
        Schema *schema = mSchemaHeap.get("String");

        size_t index = this->strings.size();
        String &str = this->strings.emplace_back(val);

        Value &value = this->values.emplace_back();
        value.set(schema, u32_t(index));

        return &value;
    }

    Value *ValueHeap::make(Schema *schema)
    {
        if (schema == nullptr)
            return nullptr;

        if (schema->type() == SchemaType::Int)
        {
            Value &value = this->values.emplace_back();
            value.set(schema, 0);
            return &value;
        }

        if (schema->type() == SchemaType::Float)
        {
            Value &value = this->values.emplace_back();
            value.set(schema, 0.0);
            return &value;
        }

        if (schema->type() == SchemaType::Bool)
        {
            Value &value = this->values.emplace_back();
            value.set(schema, 0);
            return &value;
        }

        if (schema->is(SchemaType::String) || schema->is(SchemaType::Options))
        {
            size_t index = this->strings.size();
            String &str = this->strings.emplace_back("");

            Value &value = this->values.emplace_back();
            value.set(schema, u32_t(index));

            return &value;
        }

        if (schema->type() == SchemaType::List)
        {
            size_t index = this->lists.size();
            List &list = this->lists.emplace_back();

            Value &value = this->values.emplace_back();
            value.set(schema, u32_t(index));

            return &value;
        }

        if (schema->type() == SchemaType::Struct)
        {
            size_t index = this->objects.size();
            this->objects.emplace_back();

            Value &value = this->values.emplace_back();
            value.set(schema, u32_t(index));

            for (size_t i = 0; i < schema->getMemberCount(); i++)
            {
                auto *member = schema->getMember(i);
                Value *v = this->make(member->schema);
                this->objects[index].members.emplace(member->name, *v);
            }

            return &value;
        }

        return nullptr;
    }

    u32_t ValueHeap::count(Value *list)
    {
        if (list == nullptr)
            return 0;
        if (!list->schema->is(SchemaType::List))
            return 0;
        if (list->value.i64 < 0 || list->value.i64 >= this->lists.size())
            return 0;
        List &ref = this->lists.at(list->value.i64);
        return ref.count();
    }

    // Returned element pointers are invalidated by insert, remove, and a growing push.
    Value *ValueHeap::get(Value *list, u32_t index)
    {
        if (list == nullptr)
            return nullptr;
        if (!list->schema->is(SchemaType::List))
            return nullptr;
        if (list->value.i64 < 0 || list->value.i64 >= this->lists.size())
            return nullptr;
        List &ref = this->lists.at(list->value.i64);
        return &ref.get(index);
    }

    bool ValueHeap::set(Value *list, u32_t index, Value *val)
    {
        if (list == nullptr)
            return false;
        if (!list->schema->is(SchemaType::List))
            return false;
        if (list->value.i64 < 0 || list->value.i64 >= this->lists.size())
            return false;
        List &ref = this->lists.at(list->value.i64);
        ref.set(index, *val);
        return true;
    }

    bool ValueHeap::insert(Value *list, u32_t index, Value *val)
    {
        if (list == nullptr || val == nullptr)
            return false;
        if (!list->schema->is(SchemaType::List))
            return false;
        if (list->value.i64 < 0 || list->value.i64 >= this->lists.size())
            return false;
        List &ref = this->lists.at(list->value.i64);
        Value copy = *val;
        return ref.insert(index, copy);
    }

    bool ValueHeap::remove(Value *list, u32_t index)
    {
        if (list == nullptr)
            return false;
        if (!list->schema->is(SchemaType::List))
            return false;
        if (list->value.i64 < 0 || list->value.i64 >= this->lists.size())
            return false;
        List &ref = this->lists.at(list->value.i64);
        return ref.remove(index);
    }

    bool ValueHeap::push(Value *list, Value *val)
    {
        if (list == nullptr || val == nullptr)
            return false;
        if (!list->schema->is(SchemaType::List))
            return false;
        if (list->value.i64 < 0 || list->value.i64 >= this->lists.size())
            return false;
        List &ref = this->lists.at(list->value.i64);
        Value copy = *val;
        ref.push(copy);
        return true;
    }

    bool ValueHeap::pop(Value *list, Value *val)
    {
        if (list == nullptr)
            return false;
        if (!list->schema->is(SchemaType::List))
            return false;
        if (list->value.i64 < 0 || list->value.i64 >= this->lists.size())
            return false;
        List &ref = this->lists.at(list->value.i64);
        return ref.pop(*val);
    }

    Value *ValueHeap::get(Value *object, const String &field)
    {
        if (object == nullptr)
            return nullptr;
        if (!object->schema->is(SchemaType::Struct))
            return nullptr;
        if (object->value.i64 < 0 || object->value.i64 >= this->objects.size())
            return nullptr;
        Object &ref = this->objects.at(object->value.i64);
        return &ref.get(field);
    }

    bool ValueHeap::set(Value *object, const String &field, Value *val)
    {
        if (object == nullptr || val == nullptr)
            return false;
        if (!object->schema->is(SchemaType::Struct))
            return false;
        if (object->value.i64 < 0 || object->value.i64 >= this->objects.size())
            return false;
        Object &ref = this->objects.at(object->value.i64);
        return ref.set(field, *val);
    }

    bool ValueHeap::get(Value *ptr, i32_t &val)
    {
        if (ptr == nullptr)
            return false;
        if (!ptr->schema->is(SchemaType::Int))
            return false;
        val = (i32_t) ptr->value.i64;
        return true;
    }

    bool ValueHeap::get(Value *ptr, i64_t &val)
    {
        if (ptr == nullptr)
            return false;
        if (!ptr->schema->is(SchemaType::Int))
            return false;
        val = ptr->value.i64;
        return true;
    }

    bool ValueHeap::get(Value *ptr, f64_t &val)
    {
        if (ptr == nullptr)
            return false;
        if (!ptr->schema->is(SchemaType::Float))
            return false;
        val = ptr->value.f64;
        return true;
    }

    bool ValueHeap::get(Value *ptr, bool &val)
    {
        if (ptr == nullptr)
            return false;
        if (!ptr->schema->is(SchemaType::Bool))
            return false;
        val = ptr->value.i64 != 0;
        return true;
    }

    bool ValueHeap::get(Value *ptr, String &val)
    {
        if (ptr == nullptr)
            return false;
        if (!ptr->schema->is(SchemaType::String) && !ptr->schema->is(SchemaType::Options))
            return false;
        i64_t index = ptr->value.i64;
        if (index < 0 || index >= this->strings.size())
            return false;
        val = this->strings.at(index);
        return true;
    }

    bool ValueHeap::set(Value *ptr, i32_t val)
    {
        if (ptr == nullptr)
            return false;
        if (!ptr->schema->is(SchemaType::Int))
            return false;
        ptr->value.i64 = val;
        return true;
    }

    bool ValueHeap::set(Value *ptr, i64_t val)
    {
        if (ptr == nullptr)
            return false;
        if (!ptr->schema->is(SchemaType::Int))
            return false;
        ptr->value.i64 = val;
        return true;
    }

    bool ValueHeap::set(Value *ptr, f64_t val)
    {
        if (ptr == nullptr)
            return false;
        if (!ptr->schema->is(SchemaType::Float))
            return false;
        ptr->value.f64 = val;
        return true;
    }

    bool ValueHeap::set(Value *ptr, bool val)
    {
        if (ptr == nullptr)
            return false;
        if (!ptr->schema->is(SchemaType::Bool))
            return false;
        ptr->value.i64 = val ? 1 : 0;
        return true;
    }

    bool ValueHeap::set(Value *ptr, const String &val)
    {
        if (ptr == nullptr)
            return false;
        if (!ptr->schema->is(SchemaType::String) && !ptr->schema->is(SchemaType::Options))
            return false;

        size_t index = this->strings.size();
        for (size_t i = 0; i < this->strings.size(); i++)
        {
            if (val == this->strings.at(i))
            {
                index = i;
                break;
            }
        }
        if (index == this->strings.size())
        {
            this->strings.push_back(val);
        }

        ptr->value.u64 = u32_t(index);

        return true;
    }

    bool ValueHeap::clear(Value *value)
    {
        if (value == nullptr)
            return false;

        if (!value->schema->is(SchemaType::List))
            return false;
        if (value->value.i64 < 0 || value->value.i64 >= this->lists.size())
            return false;
        List &ref = this->lists.at(value->value.i64);
        ref.clear();
        return true;
    }

    void ValueHeap::clear()
    {
        this->values.clear();
        this->lists.clear();
        this->objects.clear();
        this->strings.clear();
    }

    bool ValueHeap::doesValueDependOnSchema(const Value *value, const Schema *schema) const
    {
        if (value == nullptr || schema == nullptr || value->schema == nullptr)
        {
            return false;
        }
        if (value->schema == schema)
        {
            return true;
        }

        const SchemaType type = value->schema->type();
        if (type == SchemaType::List)
        {
            const i64_t index = value->value.i64;
            if (index < 0 || static_cast<size_t>(index) >= this->lists.size())
            {
                return false;
            }
            const List &list = this->lists.at(static_cast<size_t>(index));
            for (const Value &element : list.elements)
            {
                if (this->doesValueDependOnSchema(&element, schema))
                {
                    return true;
                }
            }
            return false;
        }

        if (type == SchemaType::Struct)
        {
            const i64_t index = value->value.i64;
            if (index < 0 || static_cast<size_t>(index) >= this->objects.size())
            {
                return false;
            }
            const Object &object = this->objects.at(static_cast<size_t>(index));
            for (const auto &pair : object.members)
            {
                if (this->doesValueDependOnSchema(&pair.second, schema))
                {
                    return true;
                }
            }
            return false;
        }

        return false;
    }
}
