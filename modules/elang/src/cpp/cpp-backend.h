#ifndef _EOKAS_CPP_BACKEND_H_
#define _EOKAS_CPP_BACKEND_H_

#include "../sema/sema-backend.h"
#include "../sema/sema-module.h"
#include <sstream>

namespace eokas
{
    /**
     * C++20 source generator driven by the semantic model.
     *
     * Unlike the legacy AST-coupled coder, this backend consumes the fully typed
     * `sema_module_t` produced by the sema layer: every expression already
     * carries its resolved type and operator dispatch, so generation is a
     * straightforward, decision-free translation.
     */
    class cpp_backend_t : public sema_backend_t
    {
    public:
        cpp_backend_t() = default;
        ~cpp_backend_t() override = default;

        String generate(sema_module_t* module) override;
        const String& error() const override { return m_error; }

    private:
        // ---- declarations ----
        bool gen_decl(sema_decl_t* decl);
        bool gen_struct(sema_type_struct_t* type);
        bool gen_enum(sema_type_enum_t* type);
        bool gen_schema(sema_type_schema_t* type);
        bool gen_meta(sema_meta_t* meta);
        bool gen_global(sema_stmt_symbol_def_t* sym);
        bool gen_func(sema_func_t* func);

        // ---- types ----
        String gen_type(sema_type_t* type);
        String gen_func_signature(const String& name, sema_func_t* func);
        bool gen_func_body(sema_func_t* func);

        // ---- statements ----
        bool gen_stmt(sema_stmt_t* stmt);
        bool gen_block(sema_stmt_block_t* stmt);
        bool gen_symbol_def(sema_stmt_symbol_def_t* stmt);
        bool gen_assign(sema_stmt_assign_t* stmt);
        bool gen_return(sema_stmt_return_t* stmt);
        bool gen_if(sema_stmt_if_t* stmt);
        bool gen_for(sema_stmt_for_t* stmt);
        bool gen_while(sema_stmt_while_t* stmt);
        bool gen_switch(sema_stmt_switch_t* stmt);
        bool gen_invoke(sema_stmt_invoke_t* stmt);
        String gen_stmt_inline(sema_stmt_t* stmt);

        // ---- expressions ----
        String gen_expr(sema_expr_t* expr);
        String gen_func_lambda(sema_func_t* func);
        String gen_func_ref(sema_expr_func_ref_t* expr);
        String gen_object_def(sema_expr_object_def_t* expr);
        String gen_literal_string(const String& raw);

        // ---- operators ----
        static String map_binary(ast_binary_oper_t op);
        static String map_unary(ast_unary_oper_t op);

        // ---- output ----
        void line(const String& s);
        void raw(const String& s);
        String indent() const;

    private:
        std::stringstream m_out;
        int m_indent = 0;
        String m_error = "";
        sema_module_t* m_module = nullptr;
    };
}

#endif //_EOKAS_CPP_BACKEND_H_
