#include "sema-analyzer.h"

namespace eokas
{
    sema_stmt_t* sema_analyzer_t::analyze_stmt(ast_node_stmt_t* node, sema_scope_t* scope)
    {
        if (node == nullptr)
            return nullptr;

        switch (node->category)
        {
            case ast_category_t::BLOCK:
                return this->analyze_block(static_cast<ast_node_block_t*>(node), scope, true);
            case ast_category_t::SYMBOL_DEF:
                return this->analyze_symbol_def(static_cast<ast_node_symbol_def_t*>(node), scope);
            case ast_category_t::ASSIGN:
                return this->analyze_assign(static_cast<ast_node_assign_t*>(node), scope);
            case ast_category_t::RETURN:
                return this->analyze_return(static_cast<ast_node_return_t*>(node), scope);
            case ast_category_t::IF:
                return this->analyze_if(static_cast<ast_node_if_t*>(node), scope);
            case ast_category_t::FOR:
                return this->analyze_for(static_cast<ast_node_for_t*>(node), scope);
            case ast_category_t::WHILE:
                return this->analyze_while(static_cast<ast_node_while_t*>(node), scope);
            case ast_category_t::SWITCH:
                return this->analyze_switch(static_cast<ast_node_switch_t*>(node), scope);
            case ast_category_t::BREAK:
            {
                if (loopDepth <= 0 && switchDepth <= 0)
                    module->diagnostics().error("break", "'break' is only valid inside a loop or switch.");
                return module->new_stmt<sema_stmt_break_t>();
            }
            case ast_category_t::CONTINUE:
            {
                if (loopDepth <= 0)
                    module->diagnostics().error("continue", "'continue' is only valid inside a loop.");
                return module->new_stmt<sema_stmt_continue_t>();
            }
            case ast_category_t::INVOKE:
                return this->analyze_invoke(static_cast<ast_node_invoke_t*>(node), scope);
            default:
                module->diagnostics().warning("stmt", "unsupported statement (category=%d) was skipped.", (int) node->category);
                return nullptr;
        }
    }

    sema_stmt_block_t* sema_analyzer_t::analyze_block(ast_node_block_t* node, sema_scope_t* scope, bool ownScope)
    {
        auto* out = module->new_stmt<sema_stmt_block_t>();
        out->breakable = node->breakable;
        out->scope = ownScope ? module->new_scope(sema_scope_kind_t::BLOCK, scope) : scope;

        if (node->breakable)
            switchDepth += 1; // a breakable `do` block acts like a break target

        for (auto* s : node->stmts)
        {
            sema_stmt_t* r = this->analyze_stmt(s, out->scope);
            if (r != nullptr)
                out->stmts.push_back(r);
        }

        if (node->breakable)
            switchDepth -= 1;

        return out;
    }

    sema_stmt_t* sema_analyzer_t::analyze_symbol_def(ast_node_symbol_def_t* node, sema_scope_t* scope)
    {
        auto* out = module->new_stmt<sema_stmt_symbol_def_t>();
        out->name = node->name;
        out->mutability = node->variable;

        sema_type_t* declared = node->type != nullptr ? this->resolve_type(node->type, scope) : nullptr;
        sema_expr_t* value = node->value != nullptr ? this->analyze_expr(node->value, scope) : nullptr;
        out->value = value;

        sema_type_t* type = nullptr;
        if (declared != nullptr)
        {
            type = declared;
            if (value != nullptr && !this->compatible(declared, value))
                module->diagnostics().error(node->name,
                    "cannot initialize '%s' of type '%s' with a value of type '%s'.",
                    node->name.cstr(),
                    sema_type_registry_t::describe(declared).cstr(),
                    sema_type_registry_t::describe(value->type).cstr());
        }
        else if (value != nullptr)
        {
            // infer from the initializer; literals collapse to a concrete width.
            if (is_int_literal(value))
                type = registry->type_i32();
            else if (is_float_literal(value))
                type = registry->type_f64();
            else
                type = value->type;

            if (this->is_schema_type(type))
            {
                module->diagnostics().error(node->name, "cannot infer a schema type for '%s'.", node->name.cstr());
                type = registry->type_error();
            }
        }
        else
        {
            module->diagnostics().error(node->name, "'%s' needs an explicit type or an initializer.", node->name.cstr());
            type = registry->type_error();
        }
        out->type = type;

        // register the binding (locals only; globals are pre-registered).
        auto* existing = scope->get_value(node->name, false);
        if (existing == nullptr)
        {
            auto* vsym = module->new_value_symbol();
            vsym->name = node->name;
            vsym->type = type;
            vsym->mutability = node->variable;
            vsym->decl = node;
            vsym->scope = scope;
            scope->add_value(vsym);
        }
        else if (scope->get_kind() != sema_scope_kind_t::MODULE)
        {
            module->diagnostics().error(node->name, "redefinition of '%s' in the same scope.", node->name.cstr());
        }

        return out;
    }

    sema_stmt_t* sema_analyzer_t::analyze_assign(ast_node_assign_t* node, sema_scope_t* scope)
    {
        auto* out = module->new_stmt<sema_stmt_assign_t>();
        out->left = this->analyze_expr(node->left, scope);
        out->right = this->analyze_expr(node->right, scope);

        // mutability check for simple symbol targets.
        if (out->left->kind == sema_expr_kind_t::SYMBOL_REF)
        {
            auto* ref = static_cast<sema_expr_symbol_ref_t*>(out->left);
            if (ref->symbol != nullptr && !ref->symbol->mutability)
                module->diagnostics().error(ref->name, "cannot assign to immutable 'val' symbol '%s'.", ref->name.cstr());
        }

        if (out->left->type != nullptr && !this->compatible(out->left->type, out->right))
            module->diagnostics().error("assign",
                "cannot assign a value of type '%s' to a target of type '%s'.",
                sema_type_registry_t::describe(out->right->type).cstr(),
                sema_type_registry_t::describe(out->left->type).cstr());

        return out;
    }

    sema_stmt_t* sema_analyzer_t::analyze_return(ast_node_return_t* node, sema_scope_t* scope)
    {
        auto* out = module->new_stmt<sema_stmt_return_t>();
        out->value = node->value != nullptr ? this->analyze_expr(node->value, scope) : nullptr;

        sema_type_t* ret = currentReturnType != nullptr ? currentReturnType : registry->type_void();
        if (ret->is_void())
        {
            if (out->value != nullptr && !out->value->type->is_void())
                module->diagnostics().error("return", "a void function must not return a value.");
        }
        else
        {
            if (out->value == nullptr)
                module->diagnostics().error("return", "a non-void function must return a value of type '%s'.",
                    sema_type_registry_t::describe(ret).cstr());
            else if (!this->compatible(ret, out->value))
                module->diagnostics().error("return",
                    "returned value of type '%s' is incompatible with the declared return type '%s'.",
                    sema_type_registry_t::describe(out->value->type).cstr(),
                    sema_type_registry_t::describe(ret).cstr());
        }

        return out;
    }

    sema_stmt_t* sema_analyzer_t::analyze_if(ast_node_if_t* node, sema_scope_t* scope)
    {
        auto* out = module->new_stmt<sema_stmt_if_t>();
        out->cond = this->analyze_expr(node->cond, scope);
        this->expect_bool(out->cond, "if");
        out->branch_true = node->branch_true != nullptr ? this->analyze_stmt(node->branch_true, scope) : nullptr;
        out->branch_false = node->branch_false != nullptr ? this->analyze_stmt(node->branch_false, scope) : nullptr;
        return out;
    }

    sema_stmt_t* sema_analyzer_t::analyze_for(ast_node_for_t* node, sema_scope_t* scope)
    {
        auto* out = module->new_stmt<sema_stmt_for_t>();
        out->scope = module->new_scope(sema_scope_kind_t::BLOCK, scope);
        out->init = node->init != nullptr ? this->analyze_stmt(node->init, out->scope) : nullptr;
        out->cond = node->cond != nullptr ? this->analyze_expr(node->cond, out->scope) : nullptr;
        if (out->cond != nullptr)
            this->expect_bool(out->cond, "for");
        out->step = node->step != nullptr ? this->analyze_stmt(node->step, out->scope) : nullptr;

        loopDepth += 1;
        out->body = node->body != nullptr ? this->analyze_stmt(node->body, out->scope) : nullptr;
        loopDepth -= 1;

        return out;
    }

    sema_stmt_t* sema_analyzer_t::analyze_while(ast_node_while_t* node, sema_scope_t* scope)
    {
        auto* out = module->new_stmt<sema_stmt_while_t>();
        out->cond = this->analyze_expr(node->cond, scope);
        this->expect_bool(out->cond, "while");

        loopDepth += 1;
        out->body = node->body != nullptr ? this->analyze_stmt(node->body, scope) : nullptr;
        loopDepth -= 1;

        return out;
    }

    sema_stmt_t* sema_analyzer_t::analyze_switch(ast_node_switch_t* node, sema_scope_t* scope)
    {
        auto* out = module->new_stmt<sema_stmt_switch_t>();
        out->expr = this->analyze_expr(node->expr, scope);

        switchDepth += 1;
        for (auto& c : node->cases)
        {
            sema_stmt_switch_t::case_t sc;
            sc.value = this->analyze_expr(c.value, scope);
            if (sc.value->type != nullptr && out->expr->type != nullptr &&
                !sema_type_registry_t::equals(out->expr->type, sc.value->type) &&
                !is_int_literal(sc.value) && !out->expr->type->is_error())
            {
                module->diagnostics().error("switch", "case value type '%s' does not match switch expression type '%s'.",
                    sema_type_registry_t::describe(sc.value->type).cstr(),
                    sema_type_registry_t::describe(out->expr->type).cstr());
            }
            for (auto* s : c.body)
            {
                sema_stmt_t* r = this->analyze_stmt(s, scope);
                if (r != nullptr)
                    sc.body.push_back(r);
            }
            out->cases.push_back(sc);
        }
        for (auto* s : node->default_body)
        {
            sema_stmt_t* r = this->analyze_stmt(s, scope);
            if (r != nullptr)
                out->default_body.push_back(r);
        }
        switchDepth -= 1;

        return out;
    }

    sema_stmt_t* sema_analyzer_t::analyze_invoke(ast_node_invoke_t* node, sema_scope_t* scope)
    {
        auto* out = module->new_stmt<sema_stmt_invoke_t>();
        sema_expr_t* expr = this->analyze_expr(node->expr, scope);
        if (expr->kind == sema_expr_kind_t::FUNC_REF)
            out->expr = static_cast<sema_expr_func_ref_t*>(expr);
        else
            module->diagnostics().error("invoke", "invoked expression is not a function call.");
        return out;
    }
}
