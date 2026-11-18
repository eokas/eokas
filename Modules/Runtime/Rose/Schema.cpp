#include "./Schema.h"

#include <cstddef>

namespace eokas::rose
{
    Schema::Schema(SchemaType type, const String &name)
        : mType(type)
          , mName(name)
          , mBody{}
    {
        if (mType == SchemaType::List)
        {
            mBody.listBody = new ListBody();
        } else if (mType == SchemaType::Struct)
        {
            mBody.structBody = new StructBody();
        }
        else if (mType == SchemaType::Options)
        {
            mBody.optionsBody = new OptionsBody();
        }
    }

    Schema::~Schema()
    {
        if (mType == SchemaType::List)
        {
            _DeletePointer(mBody.listBody);
        }
        if (mType == SchemaType::Struct)
        {
            _DeletePointer(mBody.structBody);
        }
        if (mType == SchemaType::Options)
        {
            _DeletePointer(mBody.optionsBody);
        }
    }

    bool Schema::operator==(const Schema &other) const
    {
        return mType == other.mType && mName == other.mName;
    }

    bool Schema::operator!=(const Schema &other) const
    {
        return mType != other.mType || mName != other.mName;
    }

    void Schema::setElementSchema(Schema *schema)
    {
        if (schema == this)
            return;
        if (mType == SchemaType::List)
        {
            mBody.listBody->element = schema;
        }
    }

    Schema *Schema::getElementSchema() const
    {
        if (mType == SchemaType::List)
        {
            return mBody.listBody->element;
        }
        return nullptr;
    }

    bool Schema::addMember(const String &name, Schema *schema)
    {
        if (mType != SchemaType::Struct)
            return false;
        if (name.isEmpty() || this->getMember(name) != nullptr)
            return false;

        Member &mem = mBody.structBody->members.emplace_back();
        mem.name = name;
        mem.schema = schema;
        return true;
    }

    bool Schema::setMember(const String &name, Schema *schema)
    {
        if (mType != SchemaType::Struct)
            return false;
        if (name.isEmpty())
            return false;

        Member *exists = this->getMember(name);
        if (exists != nullptr)
        {
            exists->schema = schema;
            return true;
        }
        return this->addMember(name, schema);
    }

    void Schema::delMember(const String &name)
    {
        if (mType != SchemaType::Struct)
            return;

        auto iter = mBody.structBody->members.begin();
        while (iter != mBody.structBody->members.end())
        {
            if (iter->name == name)
            {
                iter = mBody.structBody->members.erase(iter);
                continue;
            }
            ++iter;
        }
    }

    Schema::Member *Schema::getMember(const String &name)
    {
        if (mType != SchemaType::Struct)
            return nullptr;

        for (auto &mem: mBody.structBody->members)
        {
            if (mem.name == name)
                return &mem;
        }
        return nullptr;
    }

    Schema::Member *Schema::getMember(u32_t index)
    {
        if (mType != SchemaType::Struct)
            return nullptr;
        return &mBody.structBody->members.at(index);
    }

    u32_t Schema::getMemberCount()
    {
        if (mType != SchemaType::Struct)
            return 0;
        return u32_t(mBody.structBody->members.size());
    }

    u32_t Schema::getMemberIndex(const String &name)
    {
        if (mType != SchemaType::Struct)
            return -1;
        const auto &members = mBody.structBody->members;
        for (u32_t i = 0; i < u32_t(members.size()); i++)
        {
            if (members[i].name == name)
            {
                return u32_t(i);
            }
        }
        return -1;
    }

    bool Schema::addOption(const String &name, u32_t color)
    {
        if (mType != SchemaType::Options)
            return false;
        if (name.isEmpty() || this->getOption(name) != nullptr)
            return false;

        Option &opt = mBody.optionsBody->options.emplace_back();
        opt.name = name;
        opt.color = color;
        return true;
    }

    bool Schema::setOption(const String &name, u32_t color)
    {
        if (mType != SchemaType::Options)
            return false;
        if (name.isEmpty())
            return false;

        Option *exists = this->getOption(name);
        if (exists != nullptr)
        {
            exists->color = color;
            return true;
        }
        return this->addOption(name, color);
    }

    void Schema::delOption(const String &name)
    {
        if (mType != SchemaType::Options)
            return;

        auto &options = mBody.optionsBody->options;
        auto iter = options.begin();
        while (iter != options.end())
        {
            if (iter->name == name)
            {
                iter = options.erase(iter);
                continue;
            }
            ++iter;
        }
    }

    Schema::Option *Schema::getOption(const String &name)
    {
        if (mType != SchemaType::Options)
            return nullptr;

        for (auto &opt: mBody.optionsBody->options)
        {
            if (opt.name == name)
                return &opt;
        }
        return nullptr;
    }

    Schema::Option *Schema::getOption(u32_t index)
    {
        if (mType != SchemaType::Options)
            return nullptr;
        return &mBody.optionsBody->options.at(index);
    }

    u32_t Schema::getOptionCount()
    {
        if (mType != SchemaType::Options)
            return 0;
        return u32_t(mBody.optionsBody->options.size());
    }

    u32_t Schema::getOptionIndex(const String &name)
    {
        if (mType != SchemaType::Options)
            return -1;
        const auto &options = mBody.optionsBody->options;
        for (u32_t i = 0; i < u32_t(options.size()); i++)
        {
            if (options[i].name == name)
            {
                return i;
            }
        }
        return -1;
    }

    SchemaHeap::SchemaHeap()
        : mSchemas()
    {
        this->add(SchemaType::Int, "Int");
        this->add(SchemaType::Float, "Float");
        this->add(SchemaType::Bool, "Bool");
        this->add(SchemaType::String, "String");
    }

    SchemaHeap::~SchemaHeap()
    {
        mSchemaMap.clear();
        _DeleteList(mSchemas);
    }

    Schema *SchemaHeap::add(SchemaType type, const String &name)
    {
        auto iter = mSchemaMap.find(name);
        if (iter != mSchemaMap.end())
            return nullptr;

        u32_t index = u32_t(mSchemas.size());
        Schema *&schema = mSchemas.emplace_back();
        schema = new Schema(type, name);

        mSchemaMap.insert(std::make_pair(name, index));

        return schema;
    }

    Schema *SchemaHeap::get(const String &name) const
    {
        auto iter = mSchemaMap.find(name);
        if (iter == mSchemaMap.end())
            return nullptr;
        u32_t index = iter->second;
        return mSchemas.at(index);
    }

    Schema *SchemaHeap::get(u32_t index) const
    {
        if (size_t(index) >= mSchemas.size())
            return nullptr;
        return mSchemas.at(index);
    }

    u32_t SchemaHeap::count() const
    {
        return u32_t(mSchemas.size());
    }

    u32_t SchemaHeap::indexOf(const String &name) const
    {
        auto iter = mSchemaMap.find(name);
        if (iter == mSchemaMap.end())
            return -1;
        return u32_t(iter->second);
    }

    bool SchemaHeap::rename(const String &from, const String &to)
    {
        if (from == to)
        {
            return this->get(from) != nullptr;
        }
        if (to.isEmpty())
        {
            return false;
        }
        auto iter = mSchemaMap.find(from);
        if (iter == mSchemaMap.end())
        {
            return false;
        }
        if (mSchemaMap.find(to) != mSchemaMap.end())
        {
            return false;
        }
        u32_t index = iter->second;
        Schema *schema = mSchemas.at(index);
        mSchemaMap.erase(iter);
        schema->setName(to);
        mSchemaMap.insert(std::make_pair(to, index));
        return true;
    }

    bool SchemaHeap::remove(const String &name)
    {
        auto iter = mSchemaMap.find(name);
        if (iter == mSchemaMap.end())
        {
            return false;
        }
        u32_t index = iter->second;
        Schema *schema = mSchemas.at(index);
        mSchemas.erase(mSchemas.begin() + static_cast<std::ptrdiff_t>(index));
        mSchemaMap.clear();
        for (u32_t i = 0; i < u32_t(mSchemas.size()); ++i)
        {
            mSchemaMap.insert(std::make_pair(mSchemas[i]->name(), i));
        }
        delete schema;
        return true;
    }
}
