#include "./schema.h"

namespace eokas::datapot {

    Schema::Schema(SchemaType type, const String& name)
        : mType(type)
        , mName(name) {
        if(mType == SchemaType::List) {
            mBody.listBody = new ListBody();
        }
        else if(mType == SchemaType::Struct) {
            mBody.structBody = new StructBody();
        }
    }

    Schema::~Schema() {
        if(mType == SchemaType::List) {
            _DeletePointer(mBody.listBody);
        }
        if(mType == SchemaType::Struct) {
            _DeletePointer(mBody.structBody);
        }
    }

    bool Schema::operator==(const Schema& other) const {
        return mType == other.mType && mName == other.mName;
    }

    bool Schema::operator!=(const Schema& other) const {
        return mType != other.mType || mName != other.mName;
    }

    void Schema::setElement(Schema* schema) {
        if(schema == this)
            return;
        if(mType == SchemaType::List) {
            mBody.listBody->element = schema;
        }
    }

    Schema* Schema::getElement() const {
        if(mType == SchemaType::List) {
            return mBody.listBody->element;
        }
        return nullptr;
    }

    void Schema::addMember(const String& name, Schema* schema) {
        if(mType != SchemaType::Struct)
            return;

        Member& mem = mBody.structBody->members.emplace_back();
        mem.name = name;
        mem.schema = schema;
    }

    void Schema::setMember(const String& name, Schema* schema) {
        if(mType != SchemaType::Struct)
            return;

        Member* exists = this->getMember(name);
        if(exists != nullptr) {
            exists->schema = schema;
        }
        else {
            this->addMember(name, schema);
        }
    }

    void Schema::delMember(const String& name) {
        if(mType != SchemaType::Struct)
            return;

        auto iter = mBody.structBody->members.begin();
        while(iter != mBody.structBody->members.end()) {
            if(iter->name == name) {
                iter = mBody.structBody->members.erase(iter);
                continue;
            }
            ++ iter;
        }
    }

    Schema::Member* Schema::getMember(const String& name) {
        if(mType != SchemaType::Struct)
            return nullptr;

        for(auto& mem : mBody.structBody->members) {
            if(mem.name == name)
                return &mem;
        }
        return nullptr;
    }

    Schema::Member* Schema::getMember(u32_t index) {
        if(mType != SchemaType::Struct)
            return nullptr;
        return &mBody.structBody->members.at(index);
    }
    
    u32_t Schema::getMemberCount() {
        if(mType != SchemaType::Struct)
            return 0;
        return u32_t(mBody.structBody->members.size());
    }
    
    u32_t Schema::getMemberIndex(const String& name) {
        if(mType != SchemaType::Struct)
            return -1;
        const auto& members = mBody.structBody->members;
        for(u32_t i = 0; i < u32_t(members.size()); i++) {
            if(members[i].name == name) {
                return u32_t(i);
            }
        }
        return -1;
    }

    SchemaHeap::SchemaHeap()
        : mSchemas() {

        this->add(SchemaType::Int, "Int");
        this->add(SchemaType::Float, "Float");
        this->add(SchemaType::Bool, "Bool");
        this->add(SchemaType::String, "String");
    }

    SchemaHeap::~SchemaHeap() {
        mSchemaMap.clear();
        _DeleteList(mSchemas);
    }

    Schema* SchemaHeap::add(SchemaType type, const String& name) {
        auto iter = mSchemaMap.find(name);
        if(iter != mSchemaMap.end())
            return nullptr;
        
        u32_t index = u32_t(mSchemas.size());
        Schema*& schema = mSchemas.emplace_back();
        schema = new Schema(type, name);
        
        mSchemaMap.insert(std::make_pair(name, index));

        return schema;
    }

    Schema* SchemaHeap::get(const String& name) const {
        auto iter = mSchemaMap.find(name);
        if(iter == mSchemaMap.end())
            return nullptr;
        u32_t index = iter->second;
        return mSchemas.at(index);
    }
    
    Schema* SchemaHeap::get(u32_t index) const {
        if(size_t(index) >= mSchemas.size())
            return nullptr;
        return mSchemas.at(index);
    }
    
    u32_t SchemaHeap::count() const {
        return u32_t(mSchemas.size());
    }
    
    u32_t SchemaHeap::indexOf(const String& name) const {
        auto iter = mSchemaMap.find(name);
        if(iter == mSchemaMap.end())
            return -1;
        return u32_t(iter->second);
    }
}
