#pragma once
#include "./Header.h"
#include "./Value.h"

namespace eokas::rose
{
    enum class SchemaType
    {
        None,
        Int,
        Float,
        Bool,
        String,
        List,
        Struct,
        Options,
    };

    class Schema
    {
    public:
        struct Member
        {
            String name;
            Schema* schema = nullptr;
            String label;
            u32_t color = 0; // R8G8B8A8: R|G<<8|B<<16|A<<24 (little-endian byte order)
        };

        struct Option
        {
            String name;
            String label;
            u32_t color = 0; // R8G8B8A8: R|G<<8|B<<16|A<<24 (little-endian byte order)
        };

        struct ListBody
        {
            Schema* element = nullptr;
        };

        struct StructBody
        {
            std::vector<Member> members = {};
        };

        struct OptionsBody
        {
            std::vector<Option> options = {};
        };
        
        Schema(SchemaType type, const String& name);
        virtual ~Schema();
        
        bool operator==(const Schema& other) const;
        bool operator!=(const Schema& other) const;

        SchemaType type() const { return mType; }
        String name() const { return mName; }
        void setName(const String& name) { mName = name; }
        bool is(SchemaType type) const { return mType == type; }

        void setElementSchema(Schema* schema);
        Schema* getElementSchema() const;

        bool addMember(const String& name, Schema* schema, const String& label = String::empty, u32_t color = 0);
        bool setMember(const String& name, Schema* schema, const String& label = String::empty, u32_t color = 0);
        void delMember(const String& name);
        Member* getMember(const String& name);
        Member* getMember(u32_t index);
        u32_t getMemberCount();
        u32_t getMemberIndex(const String& name);

        bool addOption(const String& name, const String& label = String::empty, u32_t color = 0);
        bool setOption(const String& name, const String& label = String::empty, u32_t color = 0);
        void delOption(const String& name);
        Option* getOption(const String& name);
        Option* getOption(u32_t index);
        u32_t getOptionCount();
        u32_t getOptionIndex(const String& name);

    private:
        SchemaType mType;
        String mName;
        union
        {
            ListBody* listBody;
            StructBody* structBody;
            OptionsBody* optionsBody;
        } mBody;
    };

    class SchemaHeap
    {
    public:
        SchemaHeap();
        virtual ~SchemaHeap();
        
        Schema* add(SchemaType type, const String& name);
        Schema* get(const String& name) const;
        Schema* get(u32_t index) const;
        u32_t count() const;
        u32_t indexOf(const String& name) const;
        bool rename(const String& from, const String& to);
        bool remove(const String& name);

    private:
        std::map<String, u32_t> mSchemaMap;
        std::vector<Schema*> mSchemas;
    };
}
