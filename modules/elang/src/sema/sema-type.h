#ifndef _EOKAS_SEMA_TYPE_H_
#define _EOKAS_SEMA_TYPE_H_

#include "sema-header.h"

namespace eokas
{
    /**
     * Base of the semantic type model. Every value position in a well-formed
     * program resolves to a concrete `sema_type_t` (never a Schema, which is a
     * compile-time constraint only).
     */
    class sema_type_t
    {
    public:
        sema_type_t(sema_type_kind_t kind, const String& name)
            : kind(kind), name(name)
        {}

        virtual ~sema_type_t() = default;

        sema_type_kind_t get_kind() const { return kind; }
        const String& get_name() const { return name; }

        bool is_primitive() const { return kind == sema_type_kind_t::PRIMITIVE; }
        bool is_func() const { return kind == sema_type_kind_t::FUNC; }
        bool is_struct() const { return kind == sema_type_kind_t::STRUCT; }
        bool is_enum() const { return kind == sema_type_kind_t::ENUM; }
        bool is_schema() const { return kind == sema_type_kind_t::SCHEMA; }
        bool is_heap() const { return kind == sema_type_kind_t::HEAP; }
        bool is_slot() const { return kind == sema_type_kind_t::SLOT; }
        bool is_generic_param() const { return kind == sema_type_kind_t::GENERIC_PARAM; }
        bool is_error() const { return kind == sema_type_kind_t::ERROR_TYPE; }

        // primitive helpers (return false for non-primitive types)
        bool is_void() const;
        bool is_bool() const;
        bool is_string() const;
        bool is_numeric() const;
        bool is_integer() const;
        bool is_float() const;

    protected:
        sema_type_kind_t kind;
        String name;
    };

    class sema_type_primitive_t : public sema_type_t
    {
    public:
        sema_type_primitive_t(sema_primitive_kind_t prim, const String& name)
            : sema_type_t(sema_type_kind_t::PRIMITIVE, name), prim(prim)
        {}

        sema_primitive_kind_t prim;
    };

    class sema_type_func_t : public sema_type_t
    {
    public:
        sema_type_func_t(const String& name)
            : sema_type_t(sema_type_kind_t::FUNC, name)
        {}

        sema_type_t* ret = nullptr;
        std::vector<sema_type_t*> params = {};
        bool varg = false;
        std::vector<String> typeParams = {};
    };

    class sema_type_struct_t : public sema_type_t
    {
    public:
        struct field_t
        {
            String name = "";
            sema_type_t* type = nullptr;
            sema_expr_t* value = nullptr; // resolved default value (optional)
            bool isConst = false;
        };

        sema_type_struct_t(const String& name)
            : sema_type_t(sema_type_kind_t::STRUCT, name)
        {}

        std::vector<String> typeParams = {};        // for a generic template
        std::vector<sema_type_t*> typeArgs = {};     // for an instantiation
        std::vector<field_t> fields = {};
        std::vector<sema_func_t*> methods = {};
        std::vector<sema_type_schema_t*> schemas = {};

        // The originating AST node (used by the analyzer to lazily complete
        // member signatures / bodies). Backends rely on the resolved data above.
        ast_node_struct_def_t* node = nullptr;

        const field_t* get_field(const String& name) const
        {
            for (const auto& f : fields)
            {
                if (f.name == name)
                    return &f;
            }
            return nullptr;
        }

        bool implements(const String& schemaName) const;
    };

    class sema_type_enum_t : public sema_type_t
    {
    public:
        sema_type_enum_t(const String& name)
            : sema_type_t(sema_type_kind_t::ENUM, name)
        {}

        std::vector<std::pair<String, i32_t>> members = {};

        bool has_member(const String& name) const
        {
            for (const auto& m : members)
            {
                if (m.first == name)
                    return true;
            }
            return false;
        }
    };

    class sema_type_schema_t : public sema_type_t
    {
    public:
        struct member_t
        {
            String name = "";
            sema_type_t* type = nullptr; // for value-field / func-value members
            bool isFunc = false;         // declared as `func name(...) -> T;`
            bool isConst = false;
        };

        sema_type_schema_t(const String& name)
            : sema_type_t(sema_type_kind_t::SCHEMA, name)
        {}

        std::vector<String> typeParams = {};
        std::vector<sema_type_schema_t*> bases = {};
        std::vector<member_t> members = {};

        ast_node_schema_def_t* node = nullptr;

        const member_t* get_member(const String& name) const
        {
            for (const auto& m : members)
            {
                if (m.name == name)
                    return &m;
            }
            for (auto* base : bases)
            {
                if (base != nullptr)
                {
                    const member_t* m = base->get_member(name);
                    if (m != nullptr)
                        return m;
                }
            }
            return nullptr;
        }
    };

    // kernel Heap<T> / Slot<T>
    class sema_type_handle_t : public sema_type_t
    {
    public:
        sema_type_handle_t(sema_type_kind_t kind, const String& name, sema_type_t* element)
            : sema_type_t(kind, name), element(element)
        {}

        sema_type_t* element = nullptr;
    };

    class sema_type_generic_param_t : public sema_type_t
    {
    public:
        sema_type_generic_param_t(const String& name)
            : sema_type_t(sema_type_kind_t::GENERIC_PARAM, name)
        {}
    };

    /**
     * Owns the canonical primitive types and provides structural type equality
     * plus assignability rules. Composite types (struct / enum / schema / func /
     * handle / generic-param) are created and owned by the module that declares
     * them; this registry only manages the shared primitives.
     */
    class sema_type_registry_t
    {
    public:
        sema_type_registry_t();
        ~sema_type_registry_t();

        sema_type_primitive_t* type_void() const { return prim_void; }
        sema_type_primitive_t* type_bool() const { return prim_bool; }
        sema_type_primitive_t* type_string() const { return prim_string; }
        sema_type_primitive_t* type_i8() const { return prim_i8; }
        sema_type_primitive_t* type_i16() const { return prim_i16; }
        sema_type_primitive_t* type_i32() const { return prim_i32; }
        sema_type_primitive_t* type_i64() const { return prim_i64; }
        sema_type_primitive_t* type_u8() const { return prim_u8; }
        sema_type_primitive_t* type_u16() const { return prim_u16; }
        sema_type_primitive_t* type_u32() const { return prim_u32; }
        sema_type_primitive_t* type_u64() const { return prim_u64; }
        sema_type_primitive_t* type_f32() const { return prim_f32; }
        sema_type_primitive_t* type_f64() const { return prim_f64; }
        sema_type_t* type_error() const { return error_type; }

        // Returns the primitive whose name matches, or nullptr.
        sema_type_primitive_t* get_primitive(const String& name) const;

        // Structural / nominal type equality (ignores generic template names).
        static bool equals(sema_type_t* a, sema_type_t* b);

        // Whether a value of type `from` may be assigned to a slot of type `to`
        // under eokas rules (strict: no implicit numeric coercion). User struct
        // Assign-contract is handled by the analyzer separately.
        static bool assignable(sema_type_t* to, sema_type_t* from);

        // Human readable type name (for diagnostics).
        static String describe(sema_type_t* t);

    private:
        sema_type_primitive_t* prim_void;
        sema_type_primitive_t* prim_bool;
        sema_type_primitive_t* prim_string;
        sema_type_primitive_t* prim_i8;
        sema_type_primitive_t* prim_i16;
        sema_type_primitive_t* prim_i32;
        sema_type_primitive_t* prim_i64;
        sema_type_primitive_t* prim_u8;
        sema_type_primitive_t* prim_u16;
        sema_type_primitive_t* prim_u32;
        sema_type_primitive_t* prim_u64;
        sema_type_primitive_t* prim_f32;
        sema_type_primitive_t* prim_f64;
        sema_type_t* error_type;

        std::vector<sema_type_t*> owned = {};
    };
}

#endif //_EOKAS_SEMA_TYPE_H_
