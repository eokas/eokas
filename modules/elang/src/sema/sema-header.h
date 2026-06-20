#ifndef _EOKAS_SEMA_HEADER_H_
#define _EOKAS_SEMA_HEADER_H_

#include <base/main.h>
#include <utility>
#include "../ast/ast.h"

namespace eokas
{
    /*
     * The semantic analysis layer ("sema") is a fully self-contained pipeline
     * stage that sits between the parser and the code generators. It consumes an
     * `ast_node_module_t` and produces a typed semantic model (types, symbols,
     * scopes, typed expressions / statements) which one or more backends can
     * translate into target code.
     *
     * This layer does NOT depend on the `omis` or the legacy AST-coupled cpp
     * coder. Its only inputs are the AST (`ast/*`) and the base library.
     */

    // ---- kinds ----
    enum class sema_type_kind_t
    {
        NONE,
        PRIMITIVE,      // i8..u64, f32, f64, bool, void, String
        FUNC,           // function type
        STRUCT,         // value-type aggregate
        ENUM,           // nominal enumeration
        SCHEMA,         // compile-time capability constraint (NOT a value type)
        HEAP,           // kernel Heap<T>
        SLOT,           // kernel Slot<T>
        GENERIC_PARAM,  // a type parameter (T) inside a generic definition
        ERROR_TYPE,     // error-recovery placeholder
    };

    enum class sema_primitive_kind_t
    {
        PRIM_VOID, PRIM_BOOL, PRIM_STRING,
        PRIM_I8, PRIM_I16, PRIM_I32, PRIM_I64,
        PRIM_U8, PRIM_U16, PRIM_U32, PRIM_U64,
        PRIM_F32, PRIM_F64,
    };

    enum class sema_symbol_kind_t
    {
        VALUE, TYPE, MODULE,
    };

    enum class sema_expr_kind_t
    {
        TRINARY, BINARY, UNARY,
        LITERAL_INT, LITERAL_FLOAT, LITERAL_BOOL, LITERAL_STRING,
        SYMBOL_REF, FUNC_REF, FUNC_DEF,
        ARRAY_REF, OBJECT_DEF, OBJECT_REF,
        ERROR_EXPR,
    };

    enum class sema_stmt_kind_t
    {
        BLOCK, SYMBOL_DEF, ASSIGN, RETURN,
        IF, FOR, WHILE, SWITCH, BREAK, CONTINUE, INVOKE,
    };

    enum class sema_decl_kind_t
    {
        STRUCT, ENUM, SCHEMA, META, FUNC, SYMBOL,
    };

    enum class sema_scope_kind_t
    {
        MODULE, FUNCTION, BLOCK, STRUCT,
    };

    enum class sema_diagnostic_level_t
    {
        INFO, WARNING, SEVERE,
    };

    // ---- forward declarations ----
    struct sema_diagnostic_t;
    class sema_diagnostics_t;

    class sema_type_t;
    class sema_type_primitive_t;
    class sema_type_func_t;
    class sema_type_struct_t;
    class sema_type_enum_t;
    class sema_type_schema_t;
    class sema_type_handle_t;
    class sema_type_generic_param_t;

    struct sema_value_symbol_t;
    struct sema_type_symbol_t;
    class sema_scope_t;

    struct sema_expr_t;
    struct sema_expr_func_ref_t;
    struct sema_stmt_t;
    struct sema_stmt_symbol_def_t;
    struct sema_func_t;
    struct sema_meta_t;
    struct sema_decl_t;

    class sema_module_t;
    class sema_program_t;
    class sema_analyzer_t;
    class sema_backend_t;
}

#endif //_EOKAS_SEMA_HEADER_H_
