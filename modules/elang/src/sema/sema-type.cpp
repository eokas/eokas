#include "sema-type.h"

namespace eokas
{
    bool sema_type_t::is_void() const
    {
        if (kind != sema_type_kind_t::PRIMITIVE)
            return false;
        return static_cast<const sema_type_primitive_t*>(this)->prim == sema_primitive_kind_t::PRIM_VOID;
    }

    bool sema_type_t::is_bool() const
    {
        if (kind != sema_type_kind_t::PRIMITIVE)
            return false;
        return static_cast<const sema_type_primitive_t*>(this)->prim == sema_primitive_kind_t::PRIM_BOOL;
    }

    bool sema_type_t::is_string() const
    {
        if (kind != sema_type_kind_t::PRIMITIVE)
            return false;
        return static_cast<const sema_type_primitive_t*>(this)->prim == sema_primitive_kind_t::PRIM_STRING;
    }

    bool sema_type_t::is_integer() const
    {
        if (kind != sema_type_kind_t::PRIMITIVE)
            return false;
        switch (static_cast<const sema_type_primitive_t*>(this)->prim)
        {
            case sema_primitive_kind_t::PRIM_I8:
            case sema_primitive_kind_t::PRIM_I16:
            case sema_primitive_kind_t::PRIM_I32:
            case sema_primitive_kind_t::PRIM_I64:
            case sema_primitive_kind_t::PRIM_U8:
            case sema_primitive_kind_t::PRIM_U16:
            case sema_primitive_kind_t::PRIM_U32:
            case sema_primitive_kind_t::PRIM_U64:
                return true;
            default:
                return false;
        }
    }

    bool sema_type_t::is_float() const
    {
        if (kind != sema_type_kind_t::PRIMITIVE)
            return false;
        auto p = static_cast<const sema_type_primitive_t*>(this)->prim;
        return p == sema_primitive_kind_t::PRIM_F32 || p == sema_primitive_kind_t::PRIM_F64;
    }

    bool sema_type_t::is_numeric() const
    {
        return this->is_integer() || this->is_float();
    }

    bool sema_type_struct_t::implements(const String& schemaName) const
    {
        for (auto* s : schemas)
        {
            if (s == nullptr)
                continue;
            if (s->get_name() == schemaName)
                return true;
            // schema inheritance: implementing a derived schema implies bases.
            for (auto* base : s->bases)
            {
                if (base != nullptr && base->get_name() == schemaName)
                    return true;
            }
        }
        return false;
    }

    /*
    ============================================================================
    ==== registry
    ============================================================================
    */
    sema_type_registry_t::sema_type_registry_t()
    {
        prim_void = new sema_type_primitive_t(sema_primitive_kind_t::PRIM_VOID, "void");
        prim_bool = new sema_type_primitive_t(sema_primitive_kind_t::PRIM_BOOL, "bool");
        prim_string = new sema_type_primitive_t(sema_primitive_kind_t::PRIM_STRING, "String");
        prim_i8 = new sema_type_primitive_t(sema_primitive_kind_t::PRIM_I8, "i8");
        prim_i16 = new sema_type_primitive_t(sema_primitive_kind_t::PRIM_I16, "i16");
        prim_i32 = new sema_type_primitive_t(sema_primitive_kind_t::PRIM_I32, "i32");
        prim_i64 = new sema_type_primitive_t(sema_primitive_kind_t::PRIM_I64, "i64");
        prim_u8 = new sema_type_primitive_t(sema_primitive_kind_t::PRIM_U8, "u8");
        prim_u16 = new sema_type_primitive_t(sema_primitive_kind_t::PRIM_U16, "u16");
        prim_u32 = new sema_type_primitive_t(sema_primitive_kind_t::PRIM_U32, "u32");
        prim_u64 = new sema_type_primitive_t(sema_primitive_kind_t::PRIM_U64, "u64");
        prim_f32 = new sema_type_primitive_t(sema_primitive_kind_t::PRIM_F32, "f32");
        prim_f64 = new sema_type_primitive_t(sema_primitive_kind_t::PRIM_F64, "f64");
        error_type = new sema_type_t(sema_type_kind_t::ERROR_TYPE, "<error>");

        owned = {
            prim_void, prim_bool, prim_string,
            prim_i8, prim_i16, prim_i32, prim_i64,
            prim_u8, prim_u16, prim_u32, prim_u64,
            prim_f32, prim_f64, error_type,
        };
    }

    sema_type_registry_t::~sema_type_registry_t()
    {
        _DeleteList(owned);
    }

    sema_type_primitive_t* sema_type_registry_t::get_primitive(const String& name) const
    {
        if (name == "void") return prim_void;
        if (name == "bool") return prim_bool;
        if (name == "String" || name == "string") return prim_string;
        if (name == "i8") return prim_i8;
        if (name == "i16") return prim_i16;
        if (name == "i32") return prim_i32;
        if (name == "i64") return prim_i64;
        if (name == "u8") return prim_u8;
        if (name == "u16") return prim_u16;
        if (name == "u32") return prim_u32;
        if (name == "u64") return prim_u64;
        if (name == "f32") return prim_f32;
        if (name == "f64") return prim_f64;
        return nullptr;
    }

    bool sema_type_registry_t::equals(sema_type_t* a, sema_type_t* b)
    {
        if (a == b)
            return true;
        if (a == nullptr || b == nullptr)
            return false;
        if (a->is_error() || b->is_error())
            return true; // suppress cascade errors
        if (a->get_kind() != b->get_kind())
            return false;

        switch (a->get_kind())
        {
            case sema_type_kind_t::PRIMITIVE:
            {
                auto pa = static_cast<sema_type_primitive_t*>(a);
                auto pb = static_cast<sema_type_primitive_t*>(b);
                return pa->prim == pb->prim;
            }
            case sema_type_kind_t::FUNC:
            {
                auto fa = static_cast<sema_type_func_t*>(a);
                auto fb = static_cast<sema_type_func_t*>(b);
                if (!equals(fa->ret, fb->ret))
                    return false;
                if (fa->params.size() != fb->params.size())
                    return false;
                if (fa->varg != fb->varg)
                    return false;
                for (size_t i = 0; i < fa->params.size(); i++)
                {
                    if (!equals(fa->params[i], fb->params[i]))
                        return false;
                }
                return true;
            }
            case sema_type_kind_t::HEAP:
            case sema_type_kind_t::SLOT:
            {
                auto ha = static_cast<sema_type_handle_t*>(a);
                auto hb = static_cast<sema_type_handle_t*>(b);
                return equals(ha->element, hb->element);
            }
            case sema_type_kind_t::STRUCT:
            {
                auto sa = static_cast<sema_type_struct_t*>(a);
                auto sb = static_cast<sema_type_struct_t*>(b);
                if (sa->get_name() != sb->get_name())
                    return false;
                if (sa->typeArgs.size() != sb->typeArgs.size())
                    return false;
                for (size_t i = 0; i < sa->typeArgs.size(); i++)
                {
                    if (!equals(sa->typeArgs[i], sb->typeArgs[i]))
                        return false;
                }
                return true;
            }
            case sema_type_kind_t::ENUM:
            case sema_type_kind_t::SCHEMA:
            case sema_type_kind_t::GENERIC_PARAM:
                return a->get_name() == b->get_name();
            default:
                return false;
        }
    }

    bool sema_type_registry_t::assignable(sema_type_t* to, sema_type_t* from)
    {
        if (to == nullptr || from == nullptr)
            return false;
        if (to->is_error() || from->is_error())
            return true;
        // eokas forbids implicit cross-type numeric coercion: assignability is
        // exact type equality (user struct Assign-contract handled elsewhere).
        return equals(to, from);
    }

    String sema_type_registry_t::describe(sema_type_t* t)
    {
        if (t == nullptr)
            return "<null>";

        switch (t->get_kind())
        {
            case sema_type_kind_t::FUNC:
            {
                auto f = static_cast<sema_type_func_t*>(t);
                String s = "func(";
                for (size_t i = 0; i < f->params.size(); i++)
                {
                    if (i > 0) s += ", ";
                    s += describe(f->params[i]);
                }
                s += ") -> ";
                s += describe(f->ret);
                return s;
            }
            case sema_type_kind_t::HEAP:
            {
                auto h = static_cast<sema_type_handle_t*>(t);
                return String::format("Heap<%s>", describe(h->element).cstr());
            }
            case sema_type_kind_t::SLOT:
            {
                auto h = static_cast<sema_type_handle_t*>(t);
                return String::format("Slot<%s>", describe(h->element).cstr());
            }
            case sema_type_kind_t::STRUCT:
            {
                auto s = static_cast<sema_type_struct_t*>(t);
                if (s->typeArgs.empty())
                    return s->get_name();
                String r = s->get_name();
                r += "<";
                for (size_t i = 0; i < s->typeArgs.size(); i++)
                {
                    if (i > 0) r += ", ";
                    r += describe(s->typeArgs[i]);
                }
                r += ">";
                return r;
            }
            default:
                return t->get_name();
        }
    }
}
