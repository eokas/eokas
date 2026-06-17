#include "sema-module.h"
#include "sema-builtins.h"

namespace eokas
{
    /*
    ============================================================================
    ==== sema_module_t
    ============================================================================
    */
    sema_module_t::sema_module_t(sema_program_t* program, const String& name)
        : program(program), name(name)
    {
        root = this->new_scope(sema_scope_kind_t::MODULE, nullptr);
    }

    sema_module_t::~sema_module_t()
    {
        // The factory pools own everything; release in dependency-safe order.
        tops.clear();
        exportedValues.clear();
        exportedTypes.clear();
        instances.clear();

        _DeleteList(stmts);
        _DeleteList(exprs);
        _DeleteList(decls);
        _DeleteList(metas);
        _DeleteList(funcs);
        _DeleteList(ownedTypes);
        _DeleteList(typeSymbols);
        _DeleteList(valueSymbols);
        _DeleteList(scopes);
    }

    /*
    ============================================================================
    ==== sema_program_t
    ============================================================================
    */
    sema_program_t::sema_program_t()
    {
        reg = new sema_type_registry_t();
        blt = new sema_builtins_t(reg);
    }

    sema_program_t::~sema_program_t()
    {
        // Modules reference builtin / registry objects via raw pointers, so they
        // must be destroyed first.
        _DeleteMap(modules);
        _DeletePointer(blt);
        _DeletePointer(reg);
    }

    sema_module_t* sema_program_t::create_module(const String& name)
    {
        auto iter = modules.find(name);
        if (iter != modules.end())
            return iter->second;

        auto* mod = new sema_module_t(this, name);
        modules.insert(std::make_pair(name, mod));
        return mod;
    }

    sema_module_t* sema_program_t::get_module(const String& name) const
    {
        auto iter = modules.find(name);
        return iter != modules.end() ? iter->second : nullptr;
    }

    sema_value_symbol_t* sema_program_t::lookup_export_value(const String& moduleName, const String& symbol) const
    {
        auto* mod = this->get_module(moduleName);
        if (mod == nullptr)
            return nullptr;
        auto iter = mod->exportedValues.find(symbol);
        return iter != mod->exportedValues.end() ? iter->second : nullptr;
    }

    sema_type_symbol_t* sema_program_t::lookup_export_type(const String& moduleName, const String& symbol) const
    {
        auto* mod = this->get_module(moduleName);
        if (mod == nullptr)
            return nullptr;
        auto iter = mod->exportedTypes.find(symbol);
        return iter != mod->exportedTypes.end() ? iter->second : nullptr;
    }
}
