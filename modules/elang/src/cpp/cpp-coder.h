
#ifndef _EOKAS_CPP_CODER_H_
#define _EOKAS_CPP_CODER_H_

#include "../ast/ast.h"
#include <sstream>

namespace eokas
{
    /**
     * C++20 source code generator.
     *
     * Translates an eokas AST module into a single self-contained C++20
     * translation unit (a `.cpp` string). The generator is fully decoupled
     * from the omis / LLVM backend: it consumes `ast_node_module_t*` directly.
     */
    class cpp_coder_t
    {
    public:
        cpp_coder_t() = default;
        ~cpp_coder_t() = default;

        // Returns the generated C++20 source. On failure returns an empty
        // string and sets the error message (see `error()`).
        String generate(ast_node_module_t* node);

        const String& error() const { return m_error; }

    private:
        // ---- top-level ----
        bool gen_toplevel(ast_node_t* node);
        bool gen_struct(ast_node_struct_def_t* node);
        bool gen_enum(ast_node_enum_def_t* node);
        bool gen_schema(ast_node_schema_def_t* node);
        bool gen_meta(ast_node_meta_def_t* node);
        bool gen_global_symbol(ast_node_symbol_def_t* node);
        bool gen_named_func(const String& name, ast_node_func_def_t* fn, bool asMain);

        // ---- types / operators ----
        String gen_type(ast_node_type_t* node);
        String gen_func_type(ast_node_type_t* node);
        String map_binary(ast_binary_oper_t op);
        String map_unary(ast_unary_oper_t op);

        // ---- statements ----
        bool gen_stmt(ast_node_stmt_t* node);
        bool gen_block(ast_node_block_t* node);
        bool gen_symbol_def(ast_node_symbol_def_t* node);
        bool gen_assign(ast_node_assign_t* node);
        bool gen_return(ast_node_return_t* node);
        bool gen_if(ast_node_if_t* node);
        bool gen_for(ast_node_for_t* node);
        bool gen_while(ast_node_while_t* node);
        bool gen_switch(ast_node_switch_t* node);
        bool gen_invoke(ast_node_invoke_t* node);

        // Inline (no trailing newline / brace) statement form, used inside
        // `for(init; cond; step)` headers.
        String gen_stmt_inline(ast_node_stmt_t* node);

        // ---- expressions ----
        String gen_expr(ast_node_expr_t* node);
        String gen_func_lambda(ast_node_func_def_t* node);
        String gen_func_ref(ast_node_func_ref_t* node);
        String gen_object_def(ast_node_object_def_t* node);

        // ---- helpers ----
        String gen_func_signature(const String& name, ast_node_func_def_t* fn);
        bool gen_func_body(ast_node_func_def_t* fn);
        String gen_literal_string(const String& raw);
        void line(const String& s);
        void raw(const String& s);
        String indent() const;

    private:
        std::stringstream m_out;
        int m_indent = 0;
        String m_error;
    };
}

#endif //_EOKAS_CPP_CODER_H_
