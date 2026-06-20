#include "sema-analyzer.h"

namespace eokas
{
    /*
    ============================================================================
    ==== type resolution
    ============================================================================
    */
    sema_type_t* sema_analyzer_t::resolve_type(ast_node_type_ref_t* node, sema_scope_t* scope)
    {
        if (node == nullptr)
            return registry->type_void();

        const String& name = node->name;

        if (name == "func")
        {
            auto* ft = module->new_type<sema_type_func_t>("func");

            sema_scope_t* typeScope = scope;
            sema_scope_t* ownedScope = nullptr;
            if (!node->generic_defs.empty())
            {
                ownedScope = module->new_scope(sema_scope_kind_t::FUNCTION, scope);
                typeScope = ownedScope;
                for (auto* gd : node->generic_defs)
                {
                    if (gd == nullptr)
                        continue;
                    ft->typeParams.push_back(gd->name);
                    auto* gp = module->new_type<sema_type_generic_param_t>(gd->name);
                    auto* sym = module->new_type_symbol();
                    sym->name = gd->name;
                    sym->type = gp;
                    typeScope->add_type(sym);

                    for (auto* c : gd->constraints)
                    {
                        sema_type_t* constraint = this->resolve_type(c, typeScope);
                        if (constraint != nullptr && !constraint->is_schema() && !constraint->is_error())
                            module->diagnostics().error(gd->name,
                                "generic constraint on '%s' must be a schema.", gd->name.cstr());
                    }
                }
            }

            for (auto* argType : node->func_args)
                ft->params.push_back(this->resolve_type(argType, typeScope));

            ft->ret = node->func_ret != nullptr
                ? this->resolve_type(node->func_ret, typeScope)
                : registry->type_void();

            return ft;
        }

        if (name == "Heap" || name == "Slot")
        {
            sema_type_t* elem = node->type_args.empty()
                ? registry->type_error()
                : this->resolve_type(node->type_args[0], scope);
            auto kind = name == "Heap" ? sema_type_kind_t::HEAP : sema_type_kind_t::SLOT;
            return module->new_type<sema_type_handle_t>(kind, name, elem);
        }

        if (auto* prim = registry->get_primitive(name))
            return prim;

        sema_type_symbol_t* sym = scope->get_type(name);
        if (sym == nullptr || sym->type == nullptr)
        {
            module->diagnostics().error(name, "unknown type '%s'.", name.cstr());
            return registry->type_error();
        }

        sema_type_t* t = sym->type;

        if (t->is_struct())
        {
            auto* st = static_cast<sema_type_struct_t*>(t);
            if (!st->typeParams.empty())
            {
                if (node->type_args.empty())
                {
                    module->diagnostics().error(name, "generic type '%s' requires type arguments.", name.cstr());
                    return registry->type_error();
                }
                std::vector<sema_type_t*> args;
                for (auto* a : node->type_args)
                    args.push_back(this->resolve_type(a, scope));
                if (args.size() != st->typeParams.size())
                    module->diagnostics().error(name, "type '%s' expects %d type arguments, got %d.",
                        name.cstr(), (int) st->typeParams.size(), (int) args.size());
                return this->instantiate_struct(st, args, scope);
            }

            if (!node->type_args.empty())
                module->diagnostics().error(name, "type '%s' is not generic.", name.cstr());
            return t;
        }

        return t; // enum / schema / generic-param
    }

    sema_type_struct_t* sema_analyzer_t::instantiate_struct(sema_type_struct_t* tpl, const std::vector<sema_type_t*>& args, sema_scope_t* scope)
    {
        String key = this->mangle_instance(tpl->get_name(), args);
        if (auto* cached = module->find_instance(key))
            return cached;

        auto* inst = module->new_type<sema_type_struct_t>(tpl->get_name());
        inst->typeArgs = args;
        inst->node = tpl->node;
        inst->methods = tpl->methods; // template methods (generic) reused for lookup
        module->cache_instance(key, inst);

        if (tpl->node == nullptr)
            return inst;

        // bind type parameters to concrete arguments in a fresh scope.
        sema_scope_t* paramScope = module->new_scope(sema_scope_kind_t::STRUCT, module->get_root());
        for (size_t i = 0; i < tpl->typeParams.size() && i < args.size(); i++)
        {
            auto* psym = module->new_type_symbol();
            psym->name = tpl->typeParams[i];
            psym->type = args[i];
            paramScope->add_type(psym);
        }

        for (auto& m : tpl->node->members)
        {
            sema_type_struct_t::field_t field;
            field.name = m.name;
            field.type = this->resolve_type(m.type, paramScope);
            field.isConst = m.isConst;
            inst->fields.push_back(field);
        }
        for (auto* schemaType : tpl->node->schemas)
        {
            sema_type_t* s = this->resolve_type(schemaType, paramScope);
            if (s != nullptr && s->is_schema())
                inst->schemas.push_back(static_cast<sema_type_schema_t*>(s));
        }

        return inst;
    }

    sema_type_t* sema_analyzer_t::substitute_type(sema_type_t* t, const std::map<String, sema_type_t*>& binding)
    {
        if (t == nullptr)
            return nullptr;

        switch (t->get_kind())
        {
            case sema_type_kind_t::GENERIC_PARAM:
            {
                auto iter = binding.find(t->get_name());
                return iter != binding.end() ? iter->second : t;
            }
            case sema_type_kind_t::HEAP:
            case sema_type_kind_t::SLOT:
            {
                auto* h = static_cast<sema_type_handle_t*>(t);
                sema_type_t* elem = this->substitute_type(h->element, binding);
                if (elem == h->element)
                    return t;
                return module->new_type<sema_type_handle_t>(t->get_kind(), t->get_name(), elem);
            }
            case sema_type_kind_t::FUNC:
            {
                auto* f = static_cast<sema_type_func_t*>(t);
                auto* nf = module->new_type<sema_type_func_t>("func");
                nf->ret = this->substitute_type(f->ret, binding);
                for (auto* p : f->params)
                    nf->params.push_back(this->substitute_type(p, binding));
                nf->varg = f->varg;
                nf->typeParams = f->typeParams;
                return nf;
            }
            default:
                return t;
        }
    }

    /*
    ============================================================================
    ==== functions
    ============================================================================
    */
    sema_func_t* sema_analyzer_t::analyze_func_signature(ast_node_func_def_t* node, const String& name, bool asMain, sema_scope_t* parentScope)
    {
        auto* func = module->new_func();
        func->name = name;
        func->node = node;
        func->isMain = asMain;

        sema_scope_t* scope = module->new_scope(sema_scope_kind_t::FUNCTION, parentScope);
        func->scope = scope;

        for (auto* gd : node->generic_defs)
        {
            if (gd == nullptr)
                continue;
            func->typeParams.push_back(gd->name);
            auto* gp = module->new_type<sema_type_generic_param_t>(gd->name);
            auto* sym = module->new_type_symbol();
            sym->name = gd->name;
            sym->type = gp;
            scope->add_type(sym);
        }

        auto* ft = module->new_type<sema_type_func_t>("func");
        ft->typeParams = func->typeParams;

        for (auto* arg : node->func_args)
        {
            if (arg == nullptr)
                continue;
            sema_func_t::param_t p;
            p.name = arg->name;
            p.type = this->resolve_type(arg->type, scope);
            p.variable = arg->variable;
            func->params.push_back(p);
            ft->params.push_back(p.type);

            if (this->is_schema_type(p.type))
                module->diagnostics().error(name, "parameter '%s' may not have a schema type.", arg->name.cstr());

            auto* vsym = module->new_value_symbol();
            vsym->name = arg->name;
            vsym->type = p.type;
            vsym->mutability = arg->variable;
            vsym->scope = scope;
            scope->add_value(vsym);
        }

        func->ret = node->func_ret != nullptr ? this->resolve_type(node->func_ret, scope) : registry->type_void();
        ft->ret = func->ret;
        func->type = ft;

        return func;
    }

    void sema_analyzer_t::analyze_func_body(sema_func_t* func)
    {
        if (func == nullptr || func->node == nullptr)
            return;

        sema_type_t* savedRet = currentReturnType;
        currentReturnType = func->ret;

        for (ast_node_stmt_t* stmt : func->node->func_body)
        {
            sema_stmt_t* s = this->analyze_stmt(stmt, func->scope);
            if (s != nullptr)
                func->body.push_back(s);
        }

        currentReturnType = savedRet;
    }

    /*
    ============================================================================
    ==== helpers
    ============================================================================
    */
    sema_expr_t* sema_analyzer_t::error_expr()
    {
        auto* e = module->new_expr<sema_expr_error_t>();
        e->type = registry->type_error();
        return e;
    }

    bool sema_analyzer_t::expect_bool(sema_expr_t* expr, const String& context)
    {
        if (expr == nullptr || expr->type == nullptr)
            return false;
        if (expr->type->is_error())
            return true;
        if (expr->type->is_bool())
            return true;
        module->diagnostics().error(context, "condition must be of type 'bool', got '%s'.",
            sema_type_registry_t::describe(expr->type).cstr());
        return false;
    }

    bool sema_analyzer_t::is_schema_type(sema_type_t* t) const
    {
        return t != nullptr && t->is_schema();
    }

    bool sema_analyzer_t::is_int_literal(sema_expr_t* expr)
    {
        return expr != nullptr && expr->kind == sema_expr_kind_t::LITERAL_INT;
    }

    bool sema_analyzer_t::is_float_literal(sema_expr_t* expr)
    {
        return expr != nullptr && expr->kind == sema_expr_kind_t::LITERAL_FLOAT;
    }

    bool sema_analyzer_t::compatible(sema_type_t* to, sema_expr_t* expr)
    {
        if (to == nullptr || expr == nullptr || expr->type == nullptr)
            return false;
        if (to->is_error() || expr->type->is_error())
            return true;
        // numeric literals carry no explicit width, so they adapt to any
        // matching numeric target type.
        if (is_int_literal(expr) && (to->is_integer() || to->is_float()))
            return true;
        if (is_float_literal(expr) && to->is_float())
            return true;
        return sema_type_registry_t::equals(to, expr->type);
    }

    String sema_analyzer_t::mangle_instance(const String& base, const std::vector<sema_type_t*>& args)
    {
        String key = base;
        key += "<";
        for (size_t i = 0; i < args.size(); i++)
        {
            if (i > 0) key += ",";
            key += sema_type_registry_t::describe(args[i]);
        }
        key += ">";
        return key;
    }
}
