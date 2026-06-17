#ifndef _EOKAS_SEMA_EXPR_H_
#define _EOKAS_SEMA_EXPR_H_

#include "sema-header.h"

namespace eokas
{
    // How an operator was resolved during analysis.
    enum class sema_oper_dispatch_t
    {
        BUILTIN,        // primitive / built-in operator semantics
        SCHEMA_METHOD,  // dispatched to a struct member (UFCS Schema method)
    };

    /**
     * Base of the typed expression model. Every node carries its resolved result
     * `type`; backends never need to re-run type analysis.
     */
    struct sema_expr_t
    {
        sema_expr_kind_t kind;
        sema_type_t* type = nullptr; // result type

        explicit sema_expr_t(sema_expr_kind_t kind)
            : kind(kind)
        {}

        virtual ~sema_expr_t() = default;
    };

    struct sema_expr_trinary_t : public sema_expr_t
    {
        sema_expr_t* cond = nullptr;
        sema_expr_t* branch_true = nullptr;
        sema_expr_t* branch_false = nullptr;

        sema_expr_trinary_t() : sema_expr_t(sema_expr_kind_t::TRINARY) {}
    };

    struct sema_expr_binary_t : public sema_expr_t
    {
        ast_binary_oper_t op = ast_binary_oper_t::UNKNOWN;
        sema_expr_t* left = nullptr;
        sema_expr_t* right = nullptr;
        sema_oper_dispatch_t dispatch = sema_oper_dispatch_t::BUILTIN;
        String schemaMethod = ""; // when dispatch == SCHEMA_METHOD

        sema_expr_binary_t() : sema_expr_t(sema_expr_kind_t::BINARY) {}
    };

    struct sema_expr_unary_t : public sema_expr_t
    {
        ast_unary_oper_t op = ast_unary_oper_t::UNKNOWN;
        sema_expr_t* operand = nullptr;
        sema_oper_dispatch_t dispatch = sema_oper_dispatch_t::BUILTIN;
        String schemaMethod = "";

        sema_expr_unary_t() : sema_expr_t(sema_expr_kind_t::UNARY) {}
    };

    struct sema_expr_literal_int_t : public sema_expr_t
    {
        i64_t value = 0;
        sema_expr_literal_int_t() : sema_expr_t(sema_expr_kind_t::LITERAL_INT) {}
    };

    struct sema_expr_literal_float_t : public sema_expr_t
    {
        f64_t value = 0;
        sema_expr_literal_float_t() : sema_expr_t(sema_expr_kind_t::LITERAL_FLOAT) {}
    };

    struct sema_expr_literal_bool_t : public sema_expr_t
    {
        bool value = false;
        sema_expr_literal_bool_t() : sema_expr_t(sema_expr_kind_t::LITERAL_BOOL) {}
    };

    struct sema_expr_literal_string_t : public sema_expr_t
    {
        String value = "";
        sema_expr_literal_string_t() : sema_expr_t(sema_expr_kind_t::LITERAL_STRING) {}
    };

    struct sema_expr_symbol_ref_t : public sema_expr_t
    {
        String name = "";
        sema_value_symbol_t* symbol = nullptr;
        sema_expr_symbol_ref_t() : sema_expr_t(sema_expr_kind_t::SYMBOL_REF) {}
    };

    struct sema_expr_func_ref_t : public sema_expr_t
    {
        sema_expr_t* callee = nullptr;
        std::vector<sema_type_t*> typeArgs = {};
        std::vector<sema_expr_t*> args = {};
        sema_expr_func_ref_t() : sema_expr_t(sema_expr_kind_t::FUNC_REF) {}
    };

    struct sema_expr_func_def_t : public sema_expr_t
    {
        sema_func_t* func = nullptr; // lambda / function value
        sema_expr_func_def_t() : sema_expr_t(sema_expr_kind_t::FUNC_DEF) {}
    };

    struct sema_expr_array_ref_t : public sema_expr_t
    {
        sema_expr_t* obj = nullptr;
        sema_expr_t* key = nullptr;
        sema_expr_array_ref_t() : sema_expr_t(sema_expr_kind_t::ARRAY_REF) {}
    };

    struct sema_expr_object_def_t : public sema_expr_t
    {
        // The struct type being constructed (resolved). `members` preserves
        // source order of the provided fields.
        std::vector<std::pair<String, sema_expr_t*>> members = {};
        sema_expr_object_def_t() : sema_expr_t(sema_expr_kind_t::OBJECT_DEF) {}
    };

    struct sema_expr_object_ref_t : public sema_expr_t
    {
        sema_expr_t* obj = nullptr;
        String key = "";
        sema_expr_object_ref_t() : sema_expr_t(sema_expr_kind_t::OBJECT_REF) {}
    };

    struct sema_expr_error_t : public sema_expr_t
    {
        sema_expr_error_t() : sema_expr_t(sema_expr_kind_t::ERROR_EXPR) {}
    };

    /**
     * A function (named top-level function, member method, or anonymous lambda).
     * Holds resolved parameter types, return type, body statements and its own
     * scope.
     */
    struct sema_func_t
    {
        struct param_t
        {
            String name = "";
            sema_type_t* type = nullptr;
            bool variable = false;
        };

        String name = "";
        sema_type_func_t* type = nullptr;  // resolved function type
        sema_type_t* ret = nullptr;
        std::vector<param_t> params = {};
        std::vector<String> typeParams = {};
        std::vector<sema_stmt_t*> body = {};
        sema_scope_t* scope = nullptr;
        bool isMain = false;
        ast_node_func_def_t* node = nullptr;
    };
}

#endif //_EOKAS_SEMA_EXPR_H_
