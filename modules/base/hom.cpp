
#include "./hom.h"

namespace eokas {
    HomNode::HomNode(HomType type)
        : mType(type)
        , mValue() {
        if(mType == HomType::Number) {
            mValue = std::make_shared<HomNumber>(0);
        }
        else if(mType == HomType::Boolean){
            mValue = std::make_shared<HomBoolean>(false);
        }
        else if(mType == HomType::String) {
            mValue = std::make_shared<HomString>("");
        }
        else if(mType == HomType::Array) {
            mValue = std::make_shared<HomArray>();
        }
        else if(mType == HomType::Object) {
            mValue = std::make_shared<HomObject>();
        }
    }
        
    HomNode::HomNode(f64_t val)
        : mType(HomType::Number)
        , mValue(std::make_shared<HomNumber>(val)) {
    }
    
    HomNode::HomNode(bool val)
        : mType(HomType::Boolean)
        , mValue(std::make_shared<HomBoolean>(val)) {
    }
        
    HomNode::HomNode(const String& val)
        : mType(HomType::String)
        , mValue(std::make_shared<HomString>(val)) {
    }
    
    HomNode::HomNode(const HomNode& other) {
        *this = other;
    }
    
    HomNode& HomNode::operator=(const HomNode& other) {
        if(this == &other)
            return *this;
        
        mType = other.mType;
        mValue = other.mValue;
        
        return *this;
    }
    
    HomType HomNode::type() const {
        return mType;
    }
    
    bool HomNode::isNull() const {
        return mType == HomType::Null;
    }
    
    bool HomNode::isNumber() const {
        return mType == HomType::Number;
    }
    
    bool HomNode::isBoolean() const {
        return mType == HomType::Boolean;
    }
    
    bool HomNode::isString() const {
        return mType == HomType::String;
    }
    
    bool HomNode::isArray() const {
        return mType == HomType::Array;
    }
    
    bool HomNode::isObject() const {
        return mType == HomType::Object;
    }
    
    f64_t HomNode::asNumber() const {
        return mType == HomType::Number ? ((HomNumber*)mValue.get())->number : 0;
    }
    
    bool HomNode::asBoolean() const {
        return mType == HomType::Boolean && ((HomBoolean*)mValue.get())->boolean;
    }
    
    String HomNode::asString() const {
        return mType == HomType::String ? ((HomString*)mValue.get())->string : "";
    }
    
    HomNode HomNode::get(size_t index) {
        if(mType != HomType::Array)
            return HomNode{HomType::Null};
        auto& array = ((HomArray*)mValue.get())->array;
        if(index >= array.size())
            return HomNode{HomType::Null};
        return array.at(index);
    }
    
    void HomNode::set(size_t index, const HomNode& val) {
        if(mType != HomType::Array)
            return;
        auto& array = ((HomArray*)mValue.get())->array;
        if(index >= array.size())
            return;
        array[index] = val;
    }
    
    void HomNode::add(const HomNode& val) {
        if(mType != HomType::Array)
            return;
        auto& array = ((HomArray*)mValue.get())->array;
        array.push_back(val);
    }
    
    void HomNode::foreach(const std::function<void(const HomNode&)>& func) const {
        if(!func || mType != HomType::Array)
            return;
        auto& array = ((HomArray*)mValue.get())->array;
        for(auto& val : array) {
            func(val);
        }
    }
    
    HomNode HomNode::get(const String& key) {
        if(mType != HomType::Object)
            return HomNode{HomType::Null};
        auto& map = ((HomObject*)mValue.get())->object;
        if(map.find(key) == map.end())
            return HomNode{HomType::Null};
        return map[key];
    }
    
    void HomNode::set(const String& key, const HomNode& val) {
        if(mType != HomType::Object)
            return;
        auto& map = ((HomObject*)mValue.get())->object;
        map[key] = val;
    }
    
    void HomNode::foreach(const std::function<void(const String& key, const HomNode& val)>& func) const {
        if(!func || mType != HomType::Object)
            return;
        auto& map = ((HomObject*)mValue.get())->object;
        for(auto& pair : map) {
            func(pair.first, pair.second);
        }
    }
}
