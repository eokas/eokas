#ifndef _EOKAS_SEMA_STMT_H_
#define _EOKAS_SEMA_STMT_H_

#include "sema-header.h"

namespace eokas
{
    /**
     * Base of the typed statement model. Statements reference resolved
     * sub-expressions / sub-statements and the scope they belong to.
     */
    struct sema_stmt_t
    {
        sema_stmt_kind_t kind;

        explicit sema_stmt_t(sema_stmt_kind_t kind)
            : kind(kind)
        {}

        virtual ~sema_stmt_t() = default;
    };

    struct sema_stmt_block_t : public sema_stmt_t
    {
        std::vector<sema_stmt_t*> stmts = {};
        bool breakable = false;
        sema_scope_t* scope = nullptr;
        sema_stmt_block_t() : sema_stmt_t(sema_stmt_kind_t::BLOCK) {}
    };

    struct sema_stmt_symbol_def_t : public sema_stmt_t
    {
        String name = "";
        sema_type_t* type = nullptr; // resolved declared/inferred type
        sema_expr_t* value = nullptr;
        bool mutability = false;     // true for `var`
        sema_stmt_symbol_def_t() : sema_stmt_t(sema_stmt_kind_t::SYMBOL_DEF) {}
    };

    struct sema_stmt_assign_t : public sema_stmt_t
    {
        sema_expr_t* left = nullptr;
        sema_expr_t* right = nullptr;
        sema_stmt_assign_t() : sema_stmt_t(sema_stmt_kind_t::ASSIGN) {}
    };

    struct sema_stmt_return_t : public sema_stmt_t
    {
        sema_expr_t* value = nullptr;
        sema_stmt_return_t() : sema_stmt_t(sema_stmt_kind_t::RETURN) {}
    };

    struct sema_stmt_if_t : public sema_stmt_t
    {
        sema_expr_t* cond = nullptr;
        sema_stmt_t* branch_true = nullptr;
        sema_stmt_t* branch_false = nullptr;
        sema_stmt_if_t() : sema_stmt_t(sema_stmt_kind_t::IF) {}
    };

    struct sema_stmt_for_t : public sema_stmt_t
    {
        sema_stmt_t* init = nullptr;
        sema_expr_t* cond = nullptr;
        sema_stmt_t* step = nullptr;
        sema_stmt_t* body = nullptr;
        sema_scope_t* scope = nullptr;
        sema_stmt_for_t() : sema_stmt_t(sema_stmt_kind_t::FOR) {}
    };

    struct sema_stmt_while_t : public sema_stmt_t
    {
        sema_expr_t* cond = nullptr;
        sema_stmt_t* body = nullptr;
        sema_stmt_while_t() : sema_stmt_t(sema_stmt_kind_t::WHILE) {}
    };

    struct sema_stmt_switch_t : public sema_stmt_t
    {
        struct case_t
        {
            sema_expr_t* value = nullptr;
            std::vector<sema_stmt_t*> body = {};
        };

        sema_expr_t* expr = nullptr;
        std::vector<case_t> cases = {};
        std::vector<sema_stmt_t*> default_body = {};
        sema_stmt_switch_t() : sema_stmt_t(sema_stmt_kind_t::SWITCH) {}
    };

    struct sema_stmt_break_t : public sema_stmt_t
    {
        sema_stmt_break_t() : sema_stmt_t(sema_stmt_kind_t::BREAK) {}
    };

    struct sema_stmt_continue_t : public sema_stmt_t
    {
        sema_stmt_continue_t() : sema_stmt_t(sema_stmt_kind_t::CONTINUE) {}
    };

    struct sema_stmt_invoke_t : public sema_stmt_t
    {
        sema_expr_func_ref_t* expr = nullptr;
        sema_stmt_invoke_t() : sema_stmt_t(sema_stmt_kind_t::INVOKE) {}
    };
}

#endif //_EOKAS_SEMA_STMT_H_
