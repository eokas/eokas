#ifndef _EOKAS_SEMA_MODULE_H_
#define _EOKAS_SEMA_MODULE_H_

#include "sema-header.h"
#include "sema-type.h"
#include "sema-symbol.h"
#include "sema-scope.h"
#include "sema-expr.h"
#include "sema-stmt.h"
#include "sema-diagnostics.h"

namespace eokas
{
    // Compile-time metadata block (`meta`); emitted by backends as a plain
    // aggregate of fields.
    struct sema_meta_t
    {
        struct field_t
        {
            String name = "";
            sema_type_t* type = nullptr;
            sema_expr_t* value = nullptr;
        };

        String name = "";
        std::vector<field_t> fields = {};
    };

    // An ordered top-level declaration of a module. Backends iterate `toplevels`
    // in source order to preserve declaration ordering.
    struct sema_decl_t
    {
        sema_decl_kind_t kind;
        sema_type_t* type = nullptr;              // STRUCT / ENUM / SCHEMA
        sema_func_t* func = nullptr;              // FUNC
        sema_stmt_symbol_def_t* symbol = nullptr; // SYMBOL (global)
        sema_meta_t* meta = nullptr;              // META
    };

    /**
     * The semantic model of a single module - the unit of work for the whole
     * sema layer. A module is self contained: it owns its scope tree, all typed
     * nodes, its diagnostics and its generic instantiation cache, and it manages
     * the memory of every object it creates.
     */
    class sema_module_t
    {
    public:
        sema_module_t(sema_program_t* program, const String& name);
        ~sema_module_t();

        sema_program_t* get_program() const { return program; }
        const String& get_name() const { return name; }

        sema_scope_t* get_root() const { return root; }
        sema_diagnostics_t& diagnostics() { return diags; }
        const sema_diagnostics_t& diagnostics() const { return diags; }

        bool ok() const { return !diags.has_errors(); }

        // ordered top-level declarations
        std::vector<sema_decl_t*>& toplevels() { return tops; }
        const std::vector<sema_decl_t*>& toplevels() const { return tops; }

        // ---- object factory (centralized ownership) ----
        sema_scope_t* new_scope(sema_scope_kind_t kind, sema_scope_t* parent)
        {
            auto* s = new sema_scope_t(kind, parent);
            scopes.push_back(s);
            if (parent != nullptr)
                parent->attach_child(s);
            return s;
        }

        sema_value_symbol_t* new_value_symbol()
        {
            auto* s = new sema_value_symbol_t();
            valueSymbols.push_back(s);
            return s;
        }

        sema_type_symbol_t* new_type_symbol()
        {
            auto* s = new sema_type_symbol_t();
            typeSymbols.push_back(s);
            return s;
        }

        sema_func_t* new_func()
        {
            auto* f = new sema_func_t();
            funcs.push_back(f);
            return f;
        }

        sema_meta_t* new_meta()
        {
            auto* m = new sema_meta_t();
            metas.push_back(m);
            return m;
        }

        sema_decl_t* new_decl(sema_decl_kind_t kind)
        {
            auto* d = new sema_decl_t();
            d->kind = kind;
            decls.push_back(d);
            return d;
        }

        template<typename T, typename... Args>
        T* new_type(Args&&... args)
        {
            T* t = new T(std::forward<Args>(args)...);
            ownedTypes.push_back(t);
            return t;
        }

        template<typename T, typename... Args>
        T* new_expr(Args&&... args)
        {
            T* e = new T(std::forward<Args>(args)...);
            exprs.push_back(e);
            return e;
        }

        template<typename T, typename... Args>
        T* new_stmt(Args&&... args)
        {
            T* s = new T(std::forward<Args>(args)...);
            stmts.push_back(s);
            return s;
        }

        // generic instantiation cache (keyed by a mangled signature)
        sema_type_struct_t* find_instance(const String& key) const
        {
            auto iter = instances.find(key);
            return iter != instances.end() ? iter->second : nullptr;
        }

        void cache_instance(const String& key, sema_type_struct_t* inst)
        {
            instances.insert(std::make_pair(key, inst));
        }

        // module exports (value + type names visible to importers)
        std::map<String, sema_value_symbol_t*> exportedValues = {};
        std::map<String, sema_type_symbol_t*> exportedTypes = {};

    private:
        sema_program_t* program;
        String name;
        sema_scope_t* root;
        sema_diagnostics_t diags = {};

        std::vector<sema_decl_t*> tops = {};

        // ownership pools
        std::vector<sema_scope_t*> scopes = {};
        std::vector<sema_value_symbol_t*> valueSymbols = {};
        std::vector<sema_type_symbol_t*> typeSymbols = {};
        std::vector<sema_func_t*> funcs = {};
        std::vector<sema_meta_t*> metas = {};
        std::vector<sema_decl_t*> decls = {};
        std::vector<sema_type_t*> ownedTypes = {};
        std::vector<sema_expr_t*> exprs = {};
        std::vector<sema_stmt_t*> stmts = {};

        std::map<String, sema_type_struct_t*> instances = {};
    };

    /**
     * Cross-module coordinator. Holds the shared primitive registry and kernel
     * builtins (visible to every module) and the registry of analyzed modules.
     * It performs no analysis itself; it only supports import resolution and
     * builtin lookup.
     */
    class sema_program_t
    {
    public:
        sema_program_t();
        ~sema_program_t();

        sema_type_registry_t& registry() { return *reg; }
        class sema_builtins_t& builtins() { return *blt; }

        sema_module_t* create_module(const String& name);
        sema_module_t* get_module(const String& name) const;

        // Looks up an exported value symbol in a previously analyzed module.
        sema_value_symbol_t* lookup_export_value(const String& moduleName, const String& symbol) const;
        sema_type_symbol_t* lookup_export_type(const String& moduleName, const String& symbol) const;

    private:
        sema_type_registry_t* reg;
        class sema_builtins_t* blt;
        std::map<String, sema_module_t*> modules = {};
    };
}

#endif //_EOKAS_SEMA_MODULE_H_
