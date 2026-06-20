#include "sema-analyzer.h"

namespace eokas
{
    namespace
    {
        // Combines two numeric operand types under literal-flexibility rules.
        // Returns the resulting concrete numeric type, or nullptr on mismatch.
        sema_type_t* numeric_combine(sema_type_registry_t* reg,
                                     sema_expr_t* l, sema_expr_t* r)
        {
            sema_type_t* lt = l->type;
            sema_type_t* rt = r->type;
            bool ll = (l->kind == sema_expr_kind_t::LITERAL_INT) || (l->kind == sema_expr_kind_t::LITERAL_FLOAT);
            bool rl = (r->kind == sema_expr_kind_t::LITERAL_INT) || (r->kind == sema_expr_kind_t::LITERAL_FLOAT);

            if (lt == nullptr || rt == nullptr)
                return nullptr;
            if (!lt->is_numeric() || !rt->is_numeric())
                return nullptr;

            if (ll && rl)
            {
                bool anyFloat = (l->kind == sema_expr_kind_t::LITERAL_FLOAT) || (r->kind == sema_expr_kind_t::LITERAL_FLOAT);
                return anyFloat ? (sema_type_t*) reg->type_f64() : (sema_type_t*) reg->type_i32();
            }
            if (ll && !rl)
                return rt; // literal adapts to the concrete operand
            if (!ll && rl)
                return lt;
            // both concrete: require exact match (no implicit coercion)
            return sema_type_registry_t::equals(lt, rt) ? lt : nullptr;
        }
    }

    /*
    ============================================================================
    ==== expression dispatch
    ============================================================================
    */
    sema_expr_t* sema_analyzer_t::analyze_expr(ast_node_expr_t* node, sema_scope_t* scope)
    {
        if (node == nullptr)
            return this->error_expr();

        switch (node->category)
        {
            case ast_category_t::EXPR_TRINARY:
                return this->analyze_trinary(static_cast<ast_node_expr_trinary_t*>(node), scope);
            case ast_category_t::EXPR_BINARY:
                return this->analyze_binary(static_cast<ast_node_expr_binary_t*>(node), scope);
            case ast_category_t::EXPR_UNARY:
                return this->analyze_unary(static_cast<ast_node_expr_unary_t*>(node), scope);
            case ast_category_t::LITERAL_INT:
            {
                auto* lit = module->new_expr<sema_expr_literal_int_t>();
                lit->value = static_cast<ast_node_literal_int_t*>(node)->value;
                lit->type = registry->type_i32();
                return lit;
            }
            case ast_category_t::LITERAL_FLOAT:
            {
                auto* lit = module->new_expr<sema_expr_literal_float_t>();
                lit->value = static_cast<ast_node_literal_float_t*>(node)->value;
                lit->type = registry->type_f64();
                return lit;
            }
            case ast_category_t::LITERAL_BOOL:
            {
                auto* lit = module->new_expr<sema_expr_literal_bool_t>();
                lit->value = static_cast<ast_node_literal_bool_t*>(node)->value;
                lit->type = registry->type_bool();
                return lit;
            }
            case ast_category_t::LITERAL_STRING:
            {
                auto* lit = module->new_expr<sema_expr_literal_string_t>();
                lit->value = static_cast<ast_node_literal_string_t*>(node)->value;
                lit->type = registry->type_string();
                return lit;
            }
            case ast_category_t::SYMBOL_REF:
                return this->analyze_symbol_ref(static_cast<ast_node_symbol_ref_t*>(node), scope);
            case ast_category_t::FUNC_REF:
                return this->analyze_func_ref(static_cast<ast_node_func_ref_t*>(node), scope);
            case ast_category_t::FUNC_DEF:
                return this->analyze_func_def(static_cast<ast_node_func_def_t*>(node), scope);
            case ast_category_t::ARRAY_REF:
                return this->analyze_array_ref(static_cast<ast_node_array_ref_t*>(node), scope);
            case ast_category_t::OBJECT_DEF:
                return this->analyze_object_def(static_cast<ast_node_object_def_t*>(node), scope);
            case ast_category_t::OBJECT_REF:
                return this->analyze_object_ref(static_cast<ast_node_object_ref_t*>(node), scope);
            default:
                module->diagnostics().error("expr", "unsupported expression (category=%d).", (int) node->category);
                return this->error_expr();
        }
    }

    sema_expr_t* sema_analyzer_t::analyze_trinary(ast_node_expr_trinary_t* node, sema_scope_t* scope)
    {
        auto* out = module->new_expr<sema_expr_trinary_t>();
        out->cond = this->analyze_expr(node->cond, scope);
        out->branch_true = this->analyze_expr(node->branch_true, scope);
        out->branch_false = this->analyze_expr(node->branch_false, scope);
        this->expect_bool(out->cond, "?:");

        if (sema_type_registry_t::equals(out->branch_true->type, out->branch_false->type))
            out->type = out->branch_true->type;
        else if (out->branch_true->type->is_error() || out->branch_false->type->is_error())
            out->type = registry->type_error();
        else
        {
            module->diagnostics().error("?:", "ternary branches have mismatched types '%s' and '%s'.",
                sema_type_registry_t::describe(out->branch_true->type).cstr(),
                sema_type_registry_t::describe(out->branch_false->type).cstr());
            out->type = out->branch_true->type;
        }
        return out;
    }

    sema_expr_t* sema_analyzer_t::analyze_binary(ast_node_expr_binary_t* node, sema_scope_t* scope)
    {
        auto* out = module->new_expr<sema_expr_binary_t>();
        out->op = node->op;
        out->left = this->analyze_expr(node->left, scope);
        out->right = this->analyze_expr(node->right, scope);
        out->type = this->check_binary_op(node, out);
        return out;
    }

    sema_expr_t* sema_analyzer_t::analyze_unary(ast_node_expr_unary_t* node, sema_scope_t* scope)
    {
        auto* out = module->new_expr<sema_expr_unary_t>();
        out->op = node->op;
        out->operand = this->analyze_expr(node->right, scope);
        out->type = this->check_unary_op(node, out);
        return out;
    }

    sema_expr_t* sema_analyzer_t::analyze_symbol_ref(ast_node_symbol_ref_t* node, sema_scope_t* scope)
    {
        auto* out = module->new_expr<sema_expr_symbol_ref_t>();
        out->name = node->name;

        sema_value_symbol_t* sym = scope->get_value(node->name);
        if (sym == nullptr)
        {
            module->diagnostics().error(node->name, "use of undeclared identifier '%s'.", node->name.cstr());
            out->type = registry->type_error();
        }
        else
        {
            out->symbol = sym;
            out->type = sym->type != nullptr ? sym->type : registry->type_error();
        }
        return out;
    }

    sema_expr_t* sema_analyzer_t::analyze_func_ref(ast_node_func_ref_t* node, sema_scope_t* scope)
    {
        auto* out = module->new_expr<sema_expr_func_ref_t>();
        out->callee = this->analyze_expr(node->func, scope);
        for (auto* t : node->type_args)
            out->typeArgs.push_back(this->resolve_type(t, scope));
        for (auto* a : node->func_args)
            out->args.push_back(this->analyze_expr(a, scope));

        // kernel builtin functions
        if (out->callee->kind == sema_expr_kind_t::SYMBOL_REF)
        {
            auto* ref = static_cast<sema_expr_symbol_ref_t*>(out->callee);
            if (ref->symbol != nullptr && ref->symbol->isBuiltin)
            {
                const String& fn = ref->name;
                if (fn == "make")
                {
                    sema_type_t* elem = !out->typeArgs.empty() ? out->typeArgs[0] : registry->type_error();
                    out->type = module->new_type<sema_type_handle_t>(sema_type_kind_t::HEAP, "Heap", elem);
                }
                else if (fn == "space_of")
                {
                    sema_type_t* elem = registry->type_error();
                    if (!out->args.empty() && out->args[0]->type->is_slot())
                        elem = static_cast<sema_type_handle_t*>(out->args[0]->type)->element;
                    out->type = module->new_type<sema_type_handle_t>(sema_type_kind_t::HEAP, "Heap", elem);
                }
                else if (fn == "is_valid")
                {
                    out->type = registry->type_bool();
                }
                else // drop
                {
                    out->type = registry->type_void();
                }
                return out;
            }
        }

        sema_type_t* calleeType = out->callee->type;
        if (calleeType == nullptr || !calleeType->is_func())
        {
            if (calleeType == nullptr || !calleeType->is_error())
                module->diagnostics().error("call", "called expression is not a function.");
            out->type = registry->type_error();
            return out;
        }

        auto* ft = static_cast<sema_type_func_t*>(calleeType);

        // build generic binding (explicit type args or inferred from arguments)
        std::map<String, sema_type_t*> binding;
        if (!ft->typeParams.empty())
        {
            if (!out->typeArgs.empty())
            {
                for (size_t i = 0; i < ft->typeParams.size() && i < out->typeArgs.size(); i++)
                    binding[ft->typeParams[i]] = out->typeArgs[i];
            }
            else
            {
                for (size_t i = 0; i < ft->params.size() && i < out->args.size(); i++)
                {
                    sema_type_t* p = ft->params[i];
                    if (p != nullptr && p->is_generic_param() && binding.find(p->get_name()) == binding.end())
                        binding[p->get_name()] = out->args[i]->type;
                }
            }
        }

        if (!ft->varg && out->args.size() != ft->params.size())
            module->diagnostics().error("call", "expected %d arguments, got %d.",
                (int) ft->params.size(), (int) out->args.size());

        for (size_t i = 0; i < ft->params.size() && i < out->args.size(); i++)
        {
            sema_type_t* expected = binding.empty() ? ft->params[i] : this->substitute_type(ft->params[i], binding);
            if (expected != nullptr && !expected->is_generic_param() && !this->compatible(expected, out->args[i]))
                module->diagnostics().error("call", "argument %d has type '%s' but '%s' was expected.",
                    (int) i + 1,
                    sema_type_registry_t::describe(out->args[i]->type).cstr(),
                    sema_type_registry_t::describe(expected).cstr());
        }

        out->type = binding.empty() ? ft->ret : this->substitute_type(ft->ret, binding);
        if (out->type == nullptr)
            out->type = registry->type_error();
        return out;
    }

    sema_expr_t* sema_analyzer_t::analyze_func_def(ast_node_func_def_t* node, sema_scope_t* scope)
    {
        auto* out = module->new_expr<sema_expr_func_def_t>();
        sema_func_t* func = this->analyze_func_signature(node, node->name, false, scope);
        this->analyze_func_body(func);
        out->func = func;
        out->type = func->type;
        return out;
    }

    sema_expr_t* sema_analyzer_t::analyze_array_ref(ast_node_array_ref_t* node, sema_scope_t* scope)
    {
        auto* out = module->new_expr<sema_expr_array_ref_t>();
        out->obj = this->analyze_expr(node->obj, scope);
        out->key = this->analyze_expr(node->key, scope);

        sema_type_t* objType = out->obj->type;
        if (objType->is_heap() || objType->is_slot())
        {
            out->type = static_cast<sema_type_handle_t*>(objType)->element;
        }
        else if (objType->is_struct())
        {
            // struct must implement IndexOp; item type is unknown without full
            // schema bookkeeping, so fall back to error-suppressing type.
            auto* st = static_cast<sema_type_struct_t*>(objType);
            if (!st->implements("IndexOp"))
                module->diagnostics().error("[]", "type '%s' is not indexable.", st->get_name().cstr());
            out->type = registry->type_error();
        }
        else if (!objType->is_error())
        {
            module->diagnostics().error("[]", "type '%s' is not indexable.",
                sema_type_registry_t::describe(objType).cstr());
            out->type = registry->type_error();
        }
        else
        {
            out->type = registry->type_error();
        }
        return out;
    }

    sema_expr_t* sema_analyzer_t::analyze_object_def(ast_node_object_def_t* node, sema_scope_t* scope)
    {
        auto* out = module->new_expr<sema_expr_object_def_t>();

        sema_type_t* type = node->type != nullptr ? this->resolve_type(node->type, scope) : nullptr;
        if (type == nullptr || !type->is_struct())
        {
            if (type == nullptr)
                module->diagnostics().error("object", "object literal requires a struct type.");
            else if (!type->is_error())
                module->diagnostics().error("object", "type '%s' is not a struct.",
                    sema_type_registry_t::describe(type).cstr());
            out->type = type != nullptr ? type : registry->type_error();
            return out;
        }

        auto* st = static_cast<sema_type_struct_t*>(type);
        out->type = st;

        // validate provided members and emit them in struct field order.
        for (auto& f : st->fields)
        {
            auto iter = node->members.find(f.name);
            if (iter == node->members.end())
                continue; // missing fields are zero-initialized
            sema_expr_t* value = this->analyze_expr(iter->second, scope);
            if (f.type != nullptr && !this->compatible(f.type, value))
                module->diagnostics().error("object", "field '%s' expects type '%s' but got '%s'.",
                    f.name.cstr(),
                    sema_type_registry_t::describe(f.type).cstr(),
                    sema_type_registry_t::describe(value->type).cstr());
            out->members.emplace_back(f.name, value);
        }
        // detect unknown fields
        for (auto& m : node->members)
        {
            if (st->get_field(m.first) == nullptr)
                module->diagnostics().error("object", "struct '%s' has no field '%s'.",
                    st->get_name().cstr(), m.first.cstr());
        }

        return out;
    }

    sema_expr_t* sema_analyzer_t::analyze_object_ref(ast_node_object_ref_t* node, sema_scope_t* scope)
    {
        auto* out = module->new_expr<sema_expr_object_ref_t>();
        out->key = node->key;

        // enum member access: `EnumName.Variant`
        if (node->obj != nullptr && node->obj->category == ast_category_t::SYMBOL_REF)
        {
            auto* sref = static_cast<ast_node_symbol_ref_t*>(node->obj);

            auto aliasIt = importAliases.find(sref->name);
            if (aliasIt != importAliases.end())
            {
                sema_module_t* dep = program->get_module(aliasIt->second);
                if (dep == nullptr)
                {
                    module->diagnostics().error(sref->name,
                        "import alias '%s' refers to module '%s' which is not available.",
                        sref->name.cstr(), aliasIt->second.cstr());
                    out->type = registry->type_error();
                    return out;
                }

                auto vIt = dep->exportedValues.find(node->key);
                if (vIt != dep->exportedValues.end())
                {
                    out->obj = module->new_expr<sema_expr_symbol_ref_t>();
                    auto* symRef = static_cast<sema_expr_symbol_ref_t*>(out->obj);
                    symRef->name = node->key;
                    symRef->symbol = vIt->second;
                    symRef->type = vIt->second->type != nullptr ? vIt->second->type : registry->type_error();
                    out->type = symRef->type;
                    return out;
                }

                auto tIt = dep->exportedTypes.find(node->key);
                if (tIt != dep->exportedTypes.end())
                {
                    out->obj = module->new_expr<sema_expr_symbol_ref_t>();
                    auto* symRef = static_cast<sema_expr_symbol_ref_t*>(out->obj);
                    symRef->name = node->key;
                    symRef->type = tIt->second->type;
                    out->type = tIt->second->type;
                    return out;
                }

                module->diagnostics().error(node->key,
                    "module '%s' has no exported symbol '%s'.",
                    aliasIt->second.cstr(), node->key.cstr());
                out->type = registry->type_error();
                return out;
            }

            if (defaultImportSegments.find(sref->name) != defaultImportSegments.end())
            {
                module->diagnostics().error(sref->name,
                    "default import must not use '%s.' qualifier; use unprefixed access or import Alias = ...",
                    sref->name.cstr());
                out->type = registry->type_error();
                return out;
            }

            if (scope->get_value(sref->name) == nullptr)
            {
                sema_type_symbol_t* tsym = scope->get_type(sref->name);
                if (tsym != nullptr && tsym->type != nullptr && tsym->type->is_enum())
                {
                    auto* en = static_cast<sema_type_enum_t*>(tsym->type);
                    if (!en->has_member(node->key))
                        module->diagnostics().error(sref->name, "enum '%s' has no member '%s'.",
                            sref->name.cstr(), node->key.cstr());
                    auto* objRef = module->new_expr<sema_expr_symbol_ref_t>();
                    objRef->name = sref->name;
                    objRef->type = en;
                    out->obj = objRef;
                    out->type = en;
                    return out;
                }
            }
        }

        out->obj = this->analyze_expr(node->obj, scope);
        sema_type_t* objType = out->obj->type;

        if (objType->is_struct())
        {
            auto* st = static_cast<sema_type_struct_t*>(objType);
            const auto* field = st->get_field(node->key);
            if (field != nullptr)
            {
                out->type = field->type;
            }
            else
            {
                sema_func_t* method = nullptr;
                for (auto* m : st->methods)
                {
                    if (m != nullptr && m->name == node->key) { method = m; break; }
                }
                if (method != nullptr)
                {
                    out->type = method->type;
                }
                else
                {
                    module->diagnostics().error(node->key, "struct '%s' has no member '%s'.",
                        st->get_name().cstr(), node->key.cstr());
                    out->type = registry->type_error();
                }
            }
        }
        else if (objType->is_heap())
        {
            if (node->key == "count") out->type = registry->type_u32();
            else if (node->key == "valid") out->type = registry->type_bool();
            else { out->type = registry->type_error(); module->diagnostics().error(node->key, "Heap has no member '%s'.", node->key.cstr()); }
        }
        else if (objType->is_slot())
        {
            if (node->key == "valid") out->type = registry->type_bool();
            else if (node->key == "owner") out->type = objType; // approximation
            else { out->type = registry->type_error(); module->diagnostics().error(node->key, "Slot has no member '%s'.", node->key.cstr()); }
        }
        else if (!objType->is_error())
        {
            module->diagnostics().error(node->key, "type '%s' has no member '%s'.",
                sema_type_registry_t::describe(objType).cstr(), node->key.cstr());
            out->type = registry->type_error();
        }
        else
        {
            out->type = registry->type_error();
        }
        return out;
    }

    /*
    ============================================================================
    ==== operator typing
    ============================================================================
    */
    sema_type_t* sema_analyzer_t::check_binary_op(ast_node_expr_binary_t* ast, sema_expr_binary_t* out)
    {
        (void) ast;
        sema_expr_t* l = out->left;
        sema_expr_t* r = out->right;
        ast_binary_oper_t op = out->op;

        if (l->type == nullptr || r->type == nullptr || l->type->is_error() || r->type->is_error())
            return registry->type_error();

        // user-defined struct operand -> dispatch through a capability schema.
        if (l->type->is_struct())
        {
            auto* st = static_cast<sema_type_struct_t*>(l->type);
            String schema = sema_builtins_t::binary_op_schema(op);
            if (!schema.isEmpty() && st->implements(schema))
            {
                out->dispatch = sema_oper_dispatch_t::SCHEMA_METHOD;
                out->schemaMethod = sema_builtins_t::binary_op_method(op);
                switch (op)
                {
                    case ast_binary_oper_t::EQ:
                    case ast_binary_oper_t::NE:
                    case ast_binary_oper_t::LT:
                    case ast_binary_oper_t::GT:
                    case ast_binary_oper_t::LE:
                    case ast_binary_oper_t::GE:
                        return registry->type_bool();
                    default:
                        return l->type;
                }
            }
            module->diagnostics().error("operator", "type '%s' does not support this operator (missing schema '%s').",
                st->get_name().cstr(), schema.cstr());
            return registry->type_error();
        }

        switch (op)
        {
            case ast_binary_oper_t::ADD:
            case ast_binary_oper_t::SUB:
            case ast_binary_oper_t::MUL:
            case ast_binary_oper_t::DIV:
            {
                sema_type_t* t = numeric_combine(registry, l, r);
                if (t == nullptr)
                {
                    module->diagnostics().error("operator", "arithmetic operands have incompatible types '%s' and '%s'.",
                        sema_type_registry_t::describe(l->type).cstr(),
                        sema_type_registry_t::describe(r->type).cstr());
                    return registry->type_error();
                }
                return t;
            }
            case ast_binary_oper_t::MOD:
            {
                sema_type_t* t = numeric_combine(registry, l, r);
                if (t == nullptr || !t->is_integer())
                {
                    module->diagnostics().error("operator", "'%%' requires matching integer operands.");
                    return registry->type_error();
                }
                return t;
            }
            case ast_binary_oper_t::EQ:
            case ast_binary_oper_t::NE:
            {
                bool ok = this->compatible(l->type, r) || this->compatible(r->type, l);
                if (!ok)
                    module->diagnostics().error("operator", "cannot compare '%s' with '%s'.",
                        sema_type_registry_t::describe(l->type).cstr(),
                        sema_type_registry_t::describe(r->type).cstr());
                return registry->type_bool();
            }
            case ast_binary_oper_t::LT:
            case ast_binary_oper_t::GT:
            case ast_binary_oper_t::LE:
            case ast_binary_oper_t::GE:
            {
                bool ok = (l->type->is_numeric() || l->type->is_string()) &&
                          (this->compatible(l->type, r) || this->compatible(r->type, l));
                if (!ok)
                    module->diagnostics().error("operator", "cannot order '%s' with '%s'.",
                        sema_type_registry_t::describe(l->type).cstr(),
                        sema_type_registry_t::describe(r->type).cstr());
                return registry->type_bool();
            }
            case ast_binary_oper_t::AND:
            case ast_binary_oper_t::OR:
            {
                if (!l->type->is_bool() || !r->type->is_bool())
                    module->diagnostics().error("operator", "logical operands must both be 'bool'.");
                return registry->type_bool();
            }
            case ast_binary_oper_t::BIT_AND:
            case ast_binary_oper_t::BIT_OR:
            case ast_binary_oper_t::BIT_XOR:
            {
                sema_type_t* t = numeric_combine(registry, l, r);
                if (t == nullptr || !t->is_integer())
                {
                    module->diagnostics().error("operator", "bitwise operands must be matching integers.");
                    return registry->type_error();
                }
                return t;
            }
            case ast_binary_oper_t::SHIFT_L:
            case ast_binary_oper_t::SHIFT_R:
            {
                if (!l->type->is_integer() || !r->type->is_integer())
                {
                    module->diagnostics().error("operator", "shift operands must be integers.");
                    return registry->type_error();
                }
                return l->type;
            }
            default:
                module->diagnostics().error("operator", "unknown binary operator.");
                return registry->type_error();
        }
    }

    sema_type_t* sema_analyzer_t::check_unary_op(ast_node_expr_unary_t* ast, sema_expr_unary_t* out)
    {
        (void) ast;
        sema_expr_t* v = out->operand;
        ast_unary_oper_t op = out->op;

        if (v->type == nullptr || v->type->is_error())
            return registry->type_error();

        if (v->type->is_struct())
        {
            auto* st = static_cast<sema_type_struct_t*>(v->type);
            String schema = sema_builtins_t::unary_op_schema(op);
            if (!schema.isEmpty() && st->implements(schema))
            {
                out->dispatch = sema_oper_dispatch_t::SCHEMA_METHOD;
                out->schemaMethod = sema_builtins_t::unary_op_method(op);
                return v->type;
            }
            module->diagnostics().error("operator", "type '%s' does not support this unary operator.", st->get_name().cstr());
            return registry->type_error();
        }

        switch (op)
        {
            case ast_unary_oper_t::POS:
            case ast_unary_oper_t::NEG:
                if (!v->type->is_numeric())
                    module->diagnostics().error("operator", "unary +/- requires a numeric operand.");
                return v->type;
            case ast_unary_oper_t::NOT:
                if (!v->type->is_bool())
                    module->diagnostics().error("operator", "'!' requires a 'bool' operand.");
                return registry->type_bool();
            case ast_unary_oper_t::FLIP:
                if (!v->type->is_integer())
                    module->diagnostics().error("operator", "'~' requires an integer operand.");
                return v->type;
            default:
                module->diagnostics().error("operator", "unknown unary operator.");
                return registry->type_error();
        }
    }
}
