#include "sema-analyzer.h"

namespace eokas
{
    sema_analyzer_t::sema_analyzer_t(sema_program_t* program)
        : program(program)
    {
        registry = &program->registry();
        builtins = &program->builtins();
    }

    /*
    ============================================================================
    ==== entry / passes
    ============================================================================
    */
    sema_module_t* sema_analyzer_t::analyze(ast_node_module_t* node, bool merge)
    {
        if (node == nullptr)
            return nullptr;

        String name = node->name.isEmpty() ? String("<main>") : node->name;
        merging = merge;
        if (!merge)
        {
            importAliases.clear();
            defaultImportSegments.clear();
        }

        bool isNew = program->get_module(name) == nullptr;
        module = program->create_module(name);
        if (isNew)
            builtins->inject(module);
        else if (!merge)
        {
            module->diagnostics().error(name,
                "module '%s' has already been analyzed.", name.cstr());
            merging = false;
            return nullptr;
        }

        this->resolve_imports(node);
        this->collect_decls(node);
        this->complete_types();
        this->analyze_globals();
        this->analyze_funcs();
        this->build_toplevels(node);
        this->collect_exports(node);

        merging = false;
        return module->ok() ? module : nullptr;
    }

    void sema_analyzer_t::resolve_imports(ast_node_module_t* node)
    {
        for (auto& pair : node->imports)
        {
            ast_node_import_t* imp = pair.second;
            if (imp == nullptr)
                continue;

            String fullName = imp->target;
            String alias = imp->name;

            sema_module_t* dep = program->get_module(fullName);
            if (dep == nullptr)
                dep = program->get_module(alias);
            if (dep == nullptr)
            {
                module->diagnostics().warning(alias,
                    "imported module '%s' has not been analyzed; its symbols are unavailable.",
                    fullName.cstr());
                continue;
            }

            if (!imp->alias.isEmpty())
            {
                importAliases[imp->alias] = fullName;
                continue;
            }

            defaultImportSegments.insert(alias);

            for (auto& v : dep->exportedValues)
                module->get_root()->add_value(v.second);
            for (auto& t : dep->exportedTypes)
                module->get_root()->add_type(t.second);
        }
    }

    void sema_analyzer_t::collect_decls(ast_node_module_t* node)
    {
        if (node->entry == nullptr)
        {
            module->diagnostics().error(module->get_name(), "module entry is null.");
            return;
        }

        sema_scope_t* root = module->get_root();

        for (ast_node_stmt_t* stmt : node->entry->func_body)
        {
            if (stmt == nullptr)
                continue;

            switch (stmt->category)
            {
                case ast_category_t::STRUCT_DEF:
                {
                    auto* sd = static_cast<ast_node_struct_def_t*>(stmt);
                    auto* type = module->new_type<sema_type_struct_t>(sd->name);
                    type->node = sd;
                    type->typeParams = sd->typeParams;

                    auto* sym = module->new_type_symbol();
                    sym->name = sd->name;
                    sym->type = type;
                    if (!root->add_type(sym))
                        module->diagnostics().error(sd->name, "type '%s' is already defined.", sd->name.cstr());

                    typeByNode[sd] = type;
                    structsToComplete.push_back(type);
                    break;
                }
                case ast_category_t::ENUM_DEF:
                {
                    auto* ed = static_cast<ast_node_enum_def_t*>(stmt);
                    auto* type = module->new_type<sema_type_enum_t>(ed->name);
                    for (auto& m : ed->members)
                        type->members.emplace_back(m.first, m.second);

                    auto* sym = module->new_type_symbol();
                    sym->name = ed->name;
                    sym->type = type;
                    if (!root->add_type(sym))
                        module->diagnostics().error(ed->name, "type '%s' is already defined.", ed->name.cstr());

                    typeByNode[ed] = type;
                    break;
                }
                case ast_category_t::SCHEMA_DEF:
                {
                    auto* scd = static_cast<ast_node_schema_def_t*>(stmt);
                    auto* type = module->new_type<sema_type_schema_t>(scd->name);
                    type->node = scd;
                    type->typeParams = scd->typeParams;

                    auto* sym = module->new_type_symbol();
                    sym->name = scd->name;
                    sym->type = type;
                    if (!root->add_type(sym))
                        module->diagnostics().error(scd->name, "schema '%s' is already defined.", scd->name.cstr());

                    typeByNode[scd] = type;
                    schemasToComplete.push_back(type);
                    break;
                }
                case ast_category_t::META_DEF:
                {
                    auto* md = static_cast<ast_node_meta_def_t*>(stmt);
                    pendingMetas.push_back(md);
                    break;
                }
                case ast_category_t::FUNC_DEF:
                {
                    // A top-level func is stored in entry->func_body via reinterpret
                    // (it is an expr sibling of stmt, see parser_t::parse_module).
                    auto* fd = reinterpret_cast<ast_node_func_def_t*>(stmt);
                    auto* sym = module->new_value_symbol();
                    sym->name = fd->name;
                    sym->mutability = false;
                    sym->decl = fd;
                    sym->scope = root;
                    if (!root->add_value(sym))
                        module->diagnostics().error(fd->name, "symbol '%s' is already defined.", fd->name.cstr());
                    pendingFuncs.emplace_back(fd, fd->name);
                    break;
                }
                case ast_category_t::SYMBOL_DEF:
                {
                    auto* sdf = static_cast<ast_node_symbol_def_t*>(stmt);
                    // A symbol whose value is a function definition is a named
                    // function (matches parser/coder convention).
                    if (sdf->value != nullptr && sdf->value->category == ast_category_t::FUNC_DEF)
                    {
                        auto* fd = static_cast<ast_node_func_def_t*>(sdf->value);
                        auto* sym = module->new_value_symbol();
                        sym->name = sdf->name;
                        sym->mutability = sdf->variable;
                        sym->decl = sdf;
                        sym->scope = root;
                        if (!root->add_value(sym))
                            module->diagnostics().error(sdf->name, "symbol '%s' is already defined.", sdf->name.cstr());
                        pendingFuncs.emplace_back(fd, sdf->name);
                    }
                    else
                    {
                        auto* sym = module->new_value_symbol();
                        sym->name = sdf->name;
                        sym->mutability = sdf->variable;
                        sym->decl = sdf;
                        sym->scope = root;
                        if (!root->add_value(sym))
                            module->diagnostics().error(sdf->name, "symbol '%s' is already defined.", sdf->name.cstr());
                        pendingGlobals.push_back(sdf);
                    }
                    break;
                }
                default:
                    break;
            }
        }
    }

    void sema_analyzer_t::complete_types()
    {
        for (auto* s : schemasToComplete)
            this->complete_schema(s);
        for (auto* s : structsToComplete)
            this->complete_struct(s);

        // function signatures (after types are known)
        for (auto& pf : pendingFuncs)
        {
            ast_node_func_def_t* fd = pf.first;
            const String& name = pf.second;
            bool asMain = (name == "main");
            sema_func_t* func = this->analyze_func_signature(fd, name, asMain, module->get_root());
            funcByNode[fd] = func;

            auto* sym = module->get_root()->get_value(name, false);
            if (sym != nullptr)
                sym->type = func->type;
        }

        // meta blocks
        for (auto* md : pendingMetas)
        {
            auto* meta = module->new_meta();
            meta->name = md->name;
            for (auto& f : md->fields)
            {
                sema_meta_t::field_t mf;
                mf.name = f.name;
                mf.type = this->resolve_type(f.type, module->get_root());
                mf.value = f.value != nullptr ? this->analyze_expr(f.value, module->get_root()) : nullptr;
                meta->fields.push_back(mf);
            }
            metaByNode[md] = meta;
        }
    }

    void sema_analyzer_t::complete_schema(sema_type_schema_t* type)
    {
        ast_node_schema_def_t* node = type->node;
        if (node == nullptr)
            return;

        sema_scope_t* scope = module->new_scope(sema_scope_kind_t::STRUCT, module->get_root());
        for (auto& tp : type->typeParams)
        {
            auto* gp = module->new_type<sema_type_generic_param_t>(tp);
            auto* sym = module->new_type_symbol();
            sym->name = tp;
            sym->type = gp;
            scope->add_type(sym);
        }

        for (auto* baseType : node->bases)
        {
            sema_type_t* b = this->resolve_type(baseType, scope);
            if (b != nullptr && b->is_schema())
                type->bases.push_back(static_cast<sema_type_schema_t*>(b));
            else if (b != nullptr && !b->is_error())
                module->diagnostics().error(type->get_name(), "schema base must be a schema.");
        }

        for (auto& m : node->members)
        {
            sema_type_schema_t::member_t sm;
            sm.name = m.name;
            sm.type = this->resolve_type(m.type, scope);
            sm.isFunc = false;
            sm.isConst = m.isConst;
            type->members.push_back(sm);
        }
        for (auto* method : node->methods)
        {
            if (method == nullptr)
                continue;
            sema_type_schema_t::member_t sm;
            sm.name = method->name;
            sm.type = this->analyze_func_signature(method, method->name, false, scope)->type;
            sm.isFunc = true;
            type->members.push_back(sm);
        }
    }

    void sema_analyzer_t::complete_struct(sema_type_struct_t* type)
    {
        ast_node_struct_def_t* node = type->node;
        if (node == nullptr)
            return;

        sema_scope_t* scope = module->new_scope(sema_scope_kind_t::STRUCT, module->get_root());
        for (auto& tp : type->typeParams)
        {
            auto* gp = module->new_type<sema_type_generic_param_t>(tp);
            auto* sym = module->new_type_symbol();
            sym->name = tp;
            sym->type = gp;
            scope->add_type(sym);
        }

        // implemented schemas
        for (auto* schemaType : node->schemas)
        {
            sema_type_t* s = this->resolve_type(schemaType, scope);
            if (s != nullptr && s->is_schema())
                type->schemas.push_back(static_cast<sema_type_schema_t*>(s));
            else if (s != nullptr && !s->is_error())
                module->diagnostics().error(type->get_name(),
                    "'%s' in implements-clause is not a schema.", sema_type_registry_t::describe(s).cstr());
        }

        // fields
        for (auto& m : node->members)
        {
            sema_type_struct_t::field_t field;
            field.name = m.name;
            field.type = this->resolve_type(m.type, scope);
            field.isConst = m.isConst;
            field.value = m.value != nullptr ? this->analyze_expr(m.value, scope) : nullptr;

            if (this->is_schema_type(field.type))
                module->diagnostics().error(type->get_name(),
                    "field '%s' may not have a schema type.", m.name.cstr());

            type->fields.push_back(field);
        }

        // methods: resolve signatures now; defer bodies until after all
        // top-level function signatures have been resolved (a method body may
        // call a free function declared later).
        for (auto* method : node->methods)
        {
            if (method == nullptr)
                continue;
            sema_func_t* func = this->analyze_func_signature(method, method->name, false, scope);
            // bind 'this' inside the method body scope.
            auto* thisSym = module->new_value_symbol();
            thisSym->name = "this";
            thisSym->type = type;
            thisSym->mutability = true;
            func->scope->add_value(thisSym);
            type->methods.push_back(func);
            pendingMethods.push_back(func);
        }
    }

    void sema_analyzer_t::analyze_globals()
    {
        for (auto* sdf : pendingGlobals)
        {
            auto* stmt = static_cast<sema_stmt_symbol_def_t*>(this->analyze_symbol_def(sdf, module->get_root()));
            globalByNode[sdf] = stmt;

            auto* sym = module->get_root()->get_value(sdf->name, false);
            if (sym != nullptr)
                sym->type = stmt->type;
        }
    }

    void sema_analyzer_t::analyze_funcs()
    {
        for (auto* method : pendingMethods)
        {
            if (method != nullptr)
                this->analyze_func_body(method);
        }
        for (auto& pf : pendingFuncs)
        {
            sema_func_t* func = funcByNode[pf.first];
            if (func != nullptr)
                this->analyze_func_body(func);
        }
    }

    void sema_analyzer_t::build_toplevels(ast_node_module_t* node)
    {
        if (node->entry == nullptr)
            return;

        for (ast_node_stmt_t* stmt : node->entry->func_body)
        {
            if (stmt == nullptr)
                continue;

            switch (stmt->category)
            {
                case ast_category_t::STRUCT_DEF:
                {
                    auto* d = module->new_decl(sema_decl_kind_t::STRUCT);
                    d->type = typeByNode[stmt];
                    module->toplevels().push_back(d);
                    break;
                }
                case ast_category_t::ENUM_DEF:
                {
                    auto* d = module->new_decl(sema_decl_kind_t::ENUM);
                    d->type = typeByNode[stmt];
                    module->toplevels().push_back(d);
                    break;
                }
                case ast_category_t::SCHEMA_DEF:
                {
                    auto* d = module->new_decl(sema_decl_kind_t::SCHEMA);
                    d->type = typeByNode[stmt];
                    module->toplevels().push_back(d);
                    break;
                }
                case ast_category_t::META_DEF:
                {
                    auto* d = module->new_decl(sema_decl_kind_t::META);
                    d->meta = metaByNode[stmt];
                    module->toplevels().push_back(d);
                    break;
                }
                case ast_category_t::FUNC_DEF:
                {
                    auto* fd = reinterpret_cast<ast_node_func_def_t*>(stmt);
                    auto* d = module->new_decl(sema_decl_kind_t::FUNC);
                    d->func = funcByNode[fd];
                    module->toplevels().push_back(d);
                    break;
                }
                case ast_category_t::SYMBOL_DEF:
                {
                    auto* sdf = static_cast<ast_node_symbol_def_t*>(stmt);
                    if (sdf->value != nullptr && sdf->value->category == ast_category_t::FUNC_DEF)
                    {
                        auto* d = module->new_decl(sema_decl_kind_t::FUNC);
                        d->func = funcByNode[sdf->value];
                        module->toplevels().push_back(d);
                    }
                    else
                    {
                        auto* d = module->new_decl(sema_decl_kind_t::SYMBOL);
                        d->symbol = globalByNode[sdf];
                        module->toplevels().push_back(d);
                    }
                    break;
                }
                default:
                    break;
            }
        }
    }

    void sema_analyzer_t::collect_exports(ast_node_module_t* node)
    {
        for (auto& pair : node->exports)
        {
            ast_node_export_t* exp = pair.second;
            if (exp == nullptr)
                continue;

            if (exp->moduleReexport)
            {
                sema_module_t* dep = program->get_module(exp->modulePath);
                if (dep == nullptr)
                {
                    module->diagnostics().error(exp->modulePath,
                        "cannot re-export module '%s': module not found.",
                        exp->modulePath.cstr());
                    continue;
                }

                for (auto& v : dep->exportedValues)
                {
                    if (module->exportedValues.find(v.first) != module->exportedValues.end())
                        module->diagnostics().error(v.first,
                            "export '%s' conflicts with an existing export.", v.first.cstr());
                    else
                        module->exportedValues[v.first] = v.second;
                }
                for (auto& t : dep->exportedTypes)
                {
                    if (module->exportedTypes.find(t.first) != module->exportedTypes.end())
                        module->diagnostics().error(t.first,
                            "export '%s' conflicts with an existing export.", t.first.cstr());
                    else
                        module->exportedTypes[t.first] = t.second;
                }
                continue;
            }

            const String& name = pair.first;
            auto* v = module->get_root()->get_value(name, false);
            if (v != nullptr)
            {
                v->exported = true;
                module->exportedValues[name] = v;
            }
            auto* t = module->get_root()->get_type(name, false);
            if (t != nullptr)
            {
                t->exported = true;
                module->exportedTypes[name] = t;
            }
        }
    }
}
