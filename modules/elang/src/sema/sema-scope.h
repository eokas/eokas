#ifndef _EOKAS_SEMA_SCOPE_H_
#define _EOKAS_SEMA_SCOPE_H_

#include "sema-header.h"
#include "sema-symbol.h"

namespace eokas
{
    /**
     * A lexical scope. Holds two separate symbol tables (values and types) and
     * forms a parent chain used for name resolution. Scopes are owned by the
     * `sema_module_t` that created them; their symbol objects are likewise owned
     * by the module factory, so the scope only stores raw pointers.
     */
    class sema_scope_t
    {
    public:
        sema_scope_t(sema_scope_kind_t kind, sema_scope_t* parent)
            : kind(kind), parent(parent)
        {}

        ~sema_scope_t()
        {
            children.clear();
            values.clear();
            types.clear();
        }

        sema_scope_kind_t get_kind() const { return kind; }
        sema_scope_t* get_parent() const { return parent; }
        const std::vector<sema_scope_t*>& get_children() const { return children; }

        void attach_child(sema_scope_t* child)
        {
            children.push_back(child);
        }

        // ---- value symbols ----
        bool add_value(sema_value_symbol_t* symbol)
        {
            if (symbol == nullptr || symbol->name.isEmpty())
                return false;
            if (values.find(symbol->name) != values.end())
                return false;
            values.insert(std::make_pair(symbol->name, symbol));
            return true;
        }

        sema_value_symbol_t* get_value(const String& name, bool lookup = true)
        {
            for (auto* scope = this; scope != nullptr; scope = scope->parent)
            {
                auto iter = scope->values.find(name);
                if (iter != scope->values.end())
                    return iter->second;
                if (!lookup)
                    break;
            }
            return nullptr;
        }

        // ---- type symbols ----
        bool add_type(sema_type_symbol_t* symbol)
        {
            if (symbol == nullptr || symbol->name.isEmpty())
                return false;
            if (types.find(symbol->name) != types.end())
                return false;
            types.insert(std::make_pair(symbol->name, symbol));
            return true;
        }

        sema_type_symbol_t* get_type(const String& name, bool lookup = true)
        {
            for (auto* scope = this; scope != nullptr; scope = scope->parent)
            {
                auto iter = scope->types.find(name);
                if (iter != scope->types.end())
                    return iter->second;
                if (!lookup)
                    break;
            }
            return nullptr;
        }

        const std::map<String, sema_value_symbol_t*>& get_values() const { return values; }
        const std::map<String, sema_type_symbol_t*>& get_types() const { return types; }

    private:
        sema_scope_kind_t kind;
        sema_scope_t* parent;
        std::vector<sema_scope_t*> children = {};
        std::map<String, sema_value_symbol_t*> values = {};
        std::map<String, sema_type_symbol_t*> types = {};
    };
}

#endif //_EOKAS_SEMA_SCOPE_H_
