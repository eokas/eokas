
#ifndef  _EOKAS_BASE_HOM_H_
#define  _EOKAS_BASE_HOM_H_

/**
 * HOM: Hierarchical Object Model.
 * It is similar with DOM (Document Object Model)
 * Widely used in JSON
 * */

#include "./string.h"
#include <utility>

namespace eokas {
    
    enum class HomType {
        Null, Number, Boolean, String, Array, Object,
    };
    
    class HomNode {
    public:
        HomNode(HomType type = HomType::Null);
        HomNode(f64_t val);
        HomNode(bool val);
        HomNode(const String& val);
        HomNode(const HomNode& other);

        HomNode& operator=(const HomNode& other);
        
        HomType type() const;
        bool isNull() const;
        bool isNumber() const;
        bool isBoolean() const;
        bool isString() const;
        bool isArray() const;
        bool isObject() const;
        
        
        f64_t asNumber() const;
        bool asBoolean() const;
        String asString() const;
        
        HomNode get(size_t index);
        void set(size_t index, const HomNode& val);
        void add(const HomNode& val);
        void foreach(const std::function<void(const HomNode& val)>& func) const;
        
        HomNode get(const String& key);
        void set(const String& key, const HomNode& val);
        void foreach(const std::function<void(const String& key, const HomNode& val)>& func) const;
    
    private:
        struct HomValue {
            virtual ~HomValue() = default;
        };
        
        struct HomNumber :public HomValue {
            f64_t number;
            HomNumber(f64_t val) :number(val) {}
        };
        
        struct HomBoolean :public HomValue {
            bool boolean;
            HomBoolean(bool val) :boolean(val) {}
        };
        
        struct HomString :public HomValue {
            String string;
            HomString(const String& val) :string(val) {}
        };
        
        struct HomArray :public HomValue {
            std::vector<HomNode> array;
            HomArray() :array() {}
        };
        
        struct HomObject :public HomValue {
            std::map<String, HomNode> object;
            HomObject() :object() {}
        };
        
        HomType mType;
        std::shared_ptr<HomValue> mValue;
    };
    
}

#endif//_EOKAS_BASE_HOM_H_
