#include "./library.h"
#include "./schema.h"
#include "./value.h"

namespace eokas::datapot {
    Library::Library(const String& name)
        : mName(name)
        , mSchemas()
        , mValues(mSchemas)
        , mRoot() { }
    
    Library::~Library() {
        mRoot.clear();
    }
    
    bool Library::load(const String& filePath) {
        FileStream file = File::open(filePath, "rb");
        if(!file.isOpen()) {
            return false;
        }
        
        BinaryStream stream(file);
        return this->load(stream);
    }
    
    bool Library::save(const String& filePath) {
        FileStream file = File::open(filePath, "wb");
        if(!file.isOpen()) {
            return false;
        }
        
        BinaryStream stream(file);
        return this->save(stream);
    }
    
    bool Library::load(BinaryStream& stream) {
        static const String MAGIC = "DATAPOT";
        static const i32_t VERSION = 1;
        
        if(!stream.isOpen()) {
            return false;
        }
        
        String magic{};
        if(!stream.read(magic))
            return false;
        i32_t version = 0;
        if(!stream.read(version))
            return false;
        if(magic != MAGIC || version != VERSION)
            return false;
        
        u32_t schemaCount = 0;
        if(!stream.read(schemaCount)) return false;
        for(u32_t schemaIndex = 0; schemaIndex < schemaCount; schemaIndex++) {
            SchemaType type;
            if(!stream.read(type)) return false;
            String name;
            if(!stream.read(name)) return false;
            
            Schema* schema = mSchemas.add(type, name);
            
            if(type == SchemaType::List) {
                u32_t elementSchemaIndex = -1;
                if(!stream.read(elementSchemaIndex)) return false;
                Schema* elementSchema = mSchemas.get(elementSchemaIndex);
                schema->setElement(elementSchema);
            }
            else if(type == SchemaType::Struct) {
                u32_t memberCount = 0;
                if(!stream.read(memberCount)) return false;
                for(u32_t memberIndex = 0; memberIndex < memberCount; memberIndex ++) {
                    String memberName;
                    u32_t memberSchemaIndex = -1;
                    if(!stream.read(memberName)) return false;
                    if(!stream.read(memberSchemaIndex)) return false;
                    Schema* memberSchema = mSchemas.get(memberSchemaIndex);
                    schema->addMember(memberName, memberSchema);
                }
            }
        }
        
        auto readValue = [this](BinaryStream& stream, Value& value)->bool {
            u32_t schemaIndex = -1;
            u64_t valueU64 = 0;
            if(!stream.read(schemaIndex)) return false;
            if(!stream.read(valueU64)) return false;
            value.schema = mSchemas.get(schemaIndex);
            value.value.u64 = valueU64;
            return true;
        };
        
        auto readValueList = [this, &readValue](BinaryStream& stream, std::vector<Value>& list)->bool {
            u32_t count = -1;
            if(!stream.read(count)) return false;
            for(u32_t index = 0; index < count; index++) {
                Value& value = list.emplace_back();
                if(!readValue(stream, value))
                    return false;
            }
            return true;
        };
        
        auto readValueMap = [this, &readValue](BinaryStream& stream, std::map<String, Value>& map)->bool {
            u32_t count = -1;
            if(!stream.read(count)) return false;
            for(u32_t index = 0; index < count; index++) {
                String name;
                if(!stream.read(name)) return false;
                Value& value = map[name];
                if(!readValue(stream, value)) return false;
            }
            return true;
        };
        
        if(!readValueList(stream, mValues.values)) {
            return false;
        }
        
        u32_t listCount = 0;
        if(!stream.read(listCount)) return false;
        for(u32_t index = 0; index < listCount; index++) {
            List& list = mValues.lists.emplace_back();
            if(!readValueList(stream, list.elements)) return false;
        }
        
        u32_t objectCount = 0;
        if(!stream.read(objectCount)) return false;
        for(u32_t index = 0; index < objectCount; index++) {
            Object& obj = mValues.objects.emplace_back();
            if(!readValueMap(stream, obj.members)) return false;
        }
        
        u32_t stringCount = 0;
        if(!stream.read(stringCount)) return false;
        for(u32_t index = 0; index < stringCount; index++) {
            String& str = mValues.strings.emplace_back();
            if(!stream.read(str)) return false;
        }
        
        u32_t rootCount = 0;
        if(!stream.read(rootCount)) return false;
        for(u32_t index = 0; index < rootCount; index++) {
            String name;
            if(!stream.read(name)) return false;
            u32_t valueIndex = -1;
            if(!stream.read(valueIndex)) return false;
            mRoot[name] = &mValues.values.at(valueIndex);
        }
        
        return true;
    }
    
    bool Library::save(BinaryStream& stream) {
        static const String MAGIC_TOKEN = "DATAPOT";
        static const i32_t VERSION = 1;
        
        if(!stream.isOpen()) {
            return false;
        }
        
        stream.write(MAGIC_TOKEN);
        stream.write(VERSION);
        
        stream.write(mSchemas.count());
        for(u32_t index = 0; index < mSchemas.count(); index++) {
            Schema* schema = mSchemas.get(index);
            
            SchemaType type = schema->type();
            stream.write(type);
            
            const String& name = schema->name();
            stream.write(name);
            
            if(type == SchemaType::List) {
                Schema* elementSchema = schema->getElement();
                stream.write(mSchemas.indexOf(elementSchema->name()));
            }
            else if(type == SchemaType::Struct) {
                u32_t memberCount = schema->getMemberCount();
                stream.write(memberCount);
                for(u32_t memberIndex = 0; memberIndex < memberCount; memberIndex++) {
                    auto* member = schema->getMember(memberIndex);
                    stream.write(member->name);
                    stream.write(mSchemas.indexOf(member->schema->name()));
                }
            }
        }
        
        auto saveValueList = [this](BinaryStream& stream, const std::vector<Value>& list) {
            stream.write(u32_t(list.size()));
            for(const auto& value : list) {
                stream.write(mSchemas.indexOf(value.schema->name()));
                stream.write(value.value.u64);
            }
        };
        auto saveValueMap = [this](BinaryStream& stream, const std::map<String, Value>& map) {
            stream.write(u32_t(map.size()));
            for(auto& pair : map) {
                stream.write(pair.first);
                stream.write(mSchemas.indexOf(pair.second.schema->name()));
                stream.write(pair.second.value.u64);
            }
        };
        
        saveValueList(stream, mValues.values);
        
        stream.write(u32_t(mValues.lists.size()));
        for(List& list : mValues.lists) {
            saveValueList(stream, list.elements);
        }
        
        stream.write(u32_t(mValues.objects.size()));
        for(Object& obj : mValues.objects) {
            saveValueMap(stream, obj.members);
        }
        
        stream.write(u32_t(mValues.strings.size()));
        for(const String& str : mValues.strings) {
            stream.write(str);
        }
        
        stream.write(u32_t(mRoot.size()));
        for(auto& pair : mRoot) {
            stream.write(pair.first);
            stream.write(mValues.indexOf(pair.second));
        }
        
        return true;
    }
    
    Schema* Library::addSchema(SchemaType type, const String& name) {
        return mSchemas.add(type, name);
    }
    
    Schema* Library::getSchema(const String& name) const {
        return mSchemas.get(name);
    }
    
    Schema* Library::getSchema(u32_t index) const {
        return mSchemas.get(index);
    }
    
    u32_t Library::getSchemaCount() const {
        return mSchemas.count();
    }
    
    u32_t Library::getSchemaIndex(const String& name) const {
        return mSchemas.indexOf(name);
    }

    void Library::getRootNames(std::vector<String>& names) const {
        for(auto& pair : mRoot) {
            names.push_back(pair.first);
        }
    }

    Value* Library::make(Schema* schema) {
        return mValues.make(schema);
    }

    Value* Library::make(i32_t val) {
        return mValues.make(val);
    }
    
    Value* Library::make(i64_t val) {
        return mValues.make(val);
    }
    
    Value* Library::make(f64_t val) {
        return mValues.make(val);
    }
    
    Value* Library::make(bool val) {
        return mValues.make(val);
    }
    
    Value* Library::make(const String& val) {
        return mValues.make(val);
    }

    bool Library::get(Value* ptr, i32_t& val) {
        return mValues.get(ptr, val);
    }

    bool Library::get(Value* ptr, i64_t& val) {
        return mValues.get(ptr, val);
    }

    bool Library::get(Value* ptr, f64_t& val) {
        return mValues.get(ptr, val);
    }

    bool Library::get(Value* ptr, bool& val) {
        return mValues.get(ptr, val);
    }

    bool Library::get(Value* ptr, String& val) {
        return mValues.get(ptr, val);
    }

    bool Library::set(Value* ptr, i32_t val) {
        return mValues.set(ptr, val);
    }

    bool Library::set(Value* ptr, i64_t val) {
        return mValues.set(ptr, val);
    }

    bool Library::set(Value* ptr, f64_t val) {
        return mValues.set(ptr, val);
    }

    bool Library::set(Value* ptr, bool val) {
        return mValues.set(ptr, val);
    }

    bool Library::set(Value* ptr, const String& val) {
        return mValues.set(ptr, val);
    }
    
    Value* Library::get(const String& name) {
        auto iter = mRoot.find(name);
        if(iter == mRoot.end())
            return nullptr;
        return iter->second;
    }

    bool Library::get(const String& name, i32_t& val) {
        Value* ptr = this->get(name);
        return mValues.get(ptr, val);
    }

    bool Library::get(const String& name, i64_t& val) {
        Value* ptr = this->get(name);
        return mValues.get(ptr, val);
    }

    bool Library::get(const String& name, f64_t& val) {
        Value* ptr = this->get(name);
        return mValues.get(ptr, val);
    }

    bool Library::get(const String& name, bool& val) {
        Value* ptr = this->get(name);
        return mValues.get(ptr, val);
    }

    bool Library::get(const String& name, String& val) {
        Value* ptr = this->get(name);
        return mValues.get(ptr, val);
    }

    void Library::set(const String& name, Value* val) {
        mRoot[name] = val;
    }

    void Library::set(const String& name, i32_t val) {
        Value* value = mValues.make(val);
        mRoot[name] = value;
    }

    void Library::set(const String& name, i64_t val) {
        Value* value = mValues.make(val);
        mRoot[name] = value;
    }

    void Library::set(const String& name, f64_t val) {
        Value* value = mValues.make(val);
        mRoot[name] = value;
    }

    void Library::set(const String& name, bool val) {
        Value* value = mValues.make(val);
        mRoot[name] = value;
    }

    void Library::set(const String& name, const String& val) {
        Value* value = mValues.make(val);
        mRoot[name] = value;
    }
    
    Value* Library::get(Value* list, u32_t index) {
        return mValues.get(list, index);
    }

    bool Library::get(Value* list, u32_t index, i32_t& val) {
        Value* ptr = mValues.get(list, index);
        return mValues.get(ptr, val);
    }

    bool Library::get(Value* list, u32_t index, i64_t& val) {
        Value* ptr = mValues.get(list, index);
        return mValues.get(ptr, val);
    }

    bool Library::get(Value* list, u32_t index, f64_t& val) {
        Value* ptr = mValues.get(list, index);
        return mValues.get(ptr, val);
    }

    bool Library::get(Value* list, u32_t index, bool& val) {
        Value* ptr = mValues.get(list, index);
        return mValues.get(ptr, val);
    }

    bool Library::get(Value* list, u32_t index, String& val) {
        Value* ptr = mValues.get(list, index);
        return mValues.get(ptr, val);
    }

    bool Library::set(Value* list, u32_t index, Value* val) {
        return mValues.set(list, index, val);
    }

    bool Library::set(Value* list, u32_t index, i32_t val) {
        Value* value = mValues.make(val);
        return mValues.set(list, index, value);
    }

    bool Library::set(Value* list, u32_t index, i64_t val) {
        Value* value = mValues.make(val);
        return mValues.set(list, index, value);
    }

    bool Library::set(Value* list, u32_t index, f64_t val) {
        Value* value = mValues.make(val);
        return mValues.set(list, index, value);
    }

    bool Library::set(Value* list, u32_t index, bool val) {
        Value* value = mValues.make(val);
        return mValues.set(list, index, value);
    }

    bool Library::set(Value* list, u32_t index, const String& val) {
        Value* value = mValues.make(val);
        return mValues.set(list, index, value);
    }

    bool Library::push(Value* list, Value* val) {
        return mValues.push(list, val);
    }

    bool Library::push(Value* list, i32_t val) {
        Value* value = mValues.make(val);
        return mValues.push(list, value);
    }
    bool Library::push(Value* list, i64_t val) {
        Value* value = mValues.make(val);
        return mValues.push(list, value);
    }

    bool Library::push(Value* list, f64_t val) {
        Value* value = mValues.make(val);
        return mValues.push(list, value);
    }

    bool Library::push(Value* list, bool val) {
        Value* value = mValues.make(val);
        return mValues.push(list, value);
    }

    bool Library::push(Value* list, const String& val) {
        Value* value = mValues.make(val);
        return mValues.push(list, value);
    }

    bool Library::pop(Value* list, Value* val) {
        return mValues.pop(list, val);
    }

    bool Library::pop(Value* list, i32_t& val) {
        Value ptr{};
        if(!mValues.pop(list, &ptr))
            return false;
        return mValues.get(&ptr, val);
    }

    bool Library::pop(Value* list, i64_t& val) {
        Value ptr{};
        if(!mValues.pop(list, &ptr))
            return false;
        return mValues.get(&ptr, val);
    }

    bool Library::pop(Value* list, f64_t& val) {
        Value ptr{};
        if(!mValues.pop(list, &ptr))
            return false;
        return mValues.get(&ptr, val);
    }

    bool Library::pop(Value* list, bool& val) {
        Value ptr{};
        if(!mValues.pop(list, &ptr))
            return false;
        return mValues.get(&ptr, val);
    }

    bool Library::pop(Value* list, String& val) {
        Value ptr{};
        if(!mValues.pop(list, &ptr))
            return false;
        return mValues.get(&ptr, val);
    }
    
    Value* Library::get(Value* object, const String& field) {
        return mValues.get(object, field);
    }

    bool Library::get(Value* object, const String& field, i32_t& val) {
        Value* ptr = mValues.get(object, field);
        return mValues.get(ptr, val);
    }

    bool Library::get(Value* object, const String& field, i64_t& val) {
        Value* ptr = mValues.get(object, field);
        return mValues.get(ptr, val);
    }

    bool Library::get(Value* object, const String& field, f64_t& val) {
        Value* ptr = mValues.get(object, field);
        return mValues.get(ptr, val);
    }

    bool Library::get(Value* object, const String& field, bool& val) {
        Value* ptr = mValues.get(object, field);
        return mValues.get(ptr, val);
    }

    bool Library::get(Value* object, const String& field, String& val) {
        Value* ptr = mValues.get(object, field);
        return mValues.get(ptr, val);
    }
    
    bool Library::set(Value* object, const String& field, Value* val) {
        return mValues.set(object, field, val);
    }

    bool Library::set(Value* object, const String& field, i32_t val) {
        Value* value = mValues.make(val);
        return mValues.set(object, field, value);
    }

    bool Library::set(Value* object, const String& field, i64_t val) {
        Value* value = mValues.make(val);
        return mValues.set(object, field, value);
    }

    bool Library::set(Value* object, const String& field, f64_t val) {
        Value* value = mValues.make(val);
        return mValues.set(object, field, value);
    }

    bool Library::set(Value* object, const String& field, bool val) {
        Value* value = mValues.make(val);
        return mValues.set(object, field, value);
    }

    bool Library::set(Value* object, const String& field, const String& val) {
        Value* value = mValues.make(val);
        return mValues.set(object, field, value);
    }
}

