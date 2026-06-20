#ifndef _EOKAS_SEMA_ANALYZER_H_
#define _EOKAS_SEMA_ANALYZER_H_

#include "sema-header.h"
#include "sema-module.h"
#include "sema-builtins.h"

#include <set>

namespace eokas
{
    /**
     * The semantic analyzer. Works on a single module at a time: it consumes an
     * `ast_node_module_t`, resolves symbols / scopes / types, performs full type
     * checking, inference and generic-instantiation recording, and produces a
     * self-contained `sema_module_t`. Imports are resolved against modules that
     * were analyzed earlier and registered in the `sema_program_t`.
     */
    class sema_analyzer_t
    {
    public:
        explicit sema_analyzer_t(sema_program_t* program);
        ~sema_analyzer_t() = default;

        // Analyzes one module (or merges a fragment into an existing module when
        // `merge` is true). Returns the analyzed module on success, or nullptr
        // if it contained errors (the module is still registered in the program
        // for diagnostic aggregation).
        sema_module_t* analyze(ast_node_module_t* node, bool merge = false);

    private:
        // ---- passes ----
        void resolve_imports(ast_node_module_t* node);
        void collect_decls(ast_node_module_t* node);
        void complete_types();
        void analyze_globals();
        void analyze_funcs();
        void build_toplevels(ast_node_module_t* node);
        void collect_exports(ast_node_module_t* node);

        // ---- type resolution ----
        sema_type_t* resolve_type(ast_node_type_ref_t* node, sema_scope_t* scope);
        sema_type_struct_t* instantiate_struct(sema_type_struct_t* tpl, const std::vector<sema_type_t*>& args, sema_scope_t* scope);

        // ---- declarations ----
        void complete_struct(sema_type_struct_t* type);
        void complete_schema(sema_type_schema_t* type);
        sema_func_t* analyze_func_signature(ast_node_func_def_t* node, const String& name, bool asMain, sema_scope_t* scope);
        void analyze_func_body(sema_func_t* func);

        // ---- statements ----
        sema_stmt_t* analyze_stmt(ast_node_stmt_t* node, sema_scope_t* scope);
        sema_stmt_block_t* analyze_block(ast_node_block_t* node, sema_scope_t* scope, bool ownScope);
        sema_stmt_t* analyze_symbol_def(ast_node_symbol_def_t* node, sema_scope_t* scope);
        sema_stmt_t* analyze_assign(ast_node_assign_t* node, sema_scope_t* scope);
        sema_stmt_t* analyze_return(ast_node_return_t* node, sema_scope_t* scope);
        sema_stmt_t* analyze_if(ast_node_if_t* node, sema_scope_t* scope);
        sema_stmt_t* analyze_for(ast_node_for_t* node, sema_scope_t* scope);
        sema_stmt_t* analyze_while(ast_node_while_t* node, sema_scope_t* scope);
        sema_stmt_t* analyze_switch(ast_node_switch_t* node, sema_scope_t* scope);
        sema_stmt_t* analyze_invoke(ast_node_invoke_t* node, sema_scope_t* scope);

        // ---- expressions ----
        sema_expr_t* analyze_expr(ast_node_expr_t* node, sema_scope_t* scope);
        sema_expr_t* analyze_trinary(ast_node_expr_trinary_t* node, sema_scope_t* scope);
        sema_expr_t* analyze_binary(ast_node_expr_binary_t* node, sema_scope_t* scope);
        sema_expr_t* analyze_unary(ast_node_expr_unary_t* node, sema_scope_t* scope);
        sema_expr_t* analyze_symbol_ref(ast_node_symbol_ref_t* node, sema_scope_t* scope);
        sema_expr_t* analyze_func_ref(ast_node_func_ref_t* node, sema_scope_t* scope);
        sema_expr_t* analyze_func_def(ast_node_func_def_t* node, sema_scope_t* scope);
        sema_expr_t* analyze_array_ref(ast_node_array_ref_t* node, sema_scope_t* scope);
        sema_expr_t* analyze_object_def(ast_node_object_def_t* node, sema_scope_t* scope);
        sema_expr_t* analyze_object_ref(ast_node_object_ref_t* node, sema_scope_t* scope);

        // ---- operator typing ----
        sema_type_t* check_binary_op(ast_node_expr_binary_t* ast, sema_expr_binary_t* out);
        sema_type_t* check_unary_op(ast_node_expr_unary_t* ast, sema_expr_unary_t* out);

        // ---- helpers ----
        sema_expr_t* error_expr();
        bool expect_bool(sema_expr_t* expr, const String& context);
        bool is_schema_type(sema_type_t* t) const;
        sema_type_t* substitute_type(sema_type_t* t, const std::map<String, sema_type_t*>& binding);
        // Whether `expr` (of type `from`) may flow into a slot of type `to`,
        // honoring numeric-literal flexibility (literals adapt to any matching
        // numeric width since the AST carries no explicit literal width).
        bool compatible(sema_type_t* to, sema_expr_t* expr);
        static bool is_int_literal(sema_expr_t* expr);
        static bool is_float_literal(sema_expr_t* expr);
        String mangle_instance(const String& base, const std::vector<sema_type_t*>& args);

    private:
        sema_program_t* program;
        sema_module_t* module = nullptr;
        sema_type_registry_t* registry = nullptr;
        sema_builtins_t* builtins = nullptr;

        // analysis context
        sema_type_t* currentReturnType = nullptr;
        int loopDepth = 0;
        int switchDepth = 0;

        // node -> result maps (used to preserve declaration order in build_toplevels)
        std::map<ast_node_t*, sema_type_t*> typeByNode = {};
        std::map<ast_node_t*, sema_func_t*> funcByNode = {};
        std::map<ast_node_t*, sema_stmt_symbol_def_t*> globalByNode = {};
        std::map<ast_node_t*, sema_meta_t*> metaByNode = {};

        // completion work lists
        std::vector<sema_type_struct_t*> structsToComplete = {};
        std::vector<sema_type_schema_t*> schemasToComplete = {};
        std::vector<std::pair<ast_node_func_def_t*, String>> pendingFuncs = {};
        std::vector<ast_node_symbol_def_t*> pendingGlobals = {};
        std::vector<ast_node_meta_def_t*> pendingMetas = {};
        std::vector<sema_func_t*> pendingMethods = {};

        std::map<String, String> importAliases = {};
        std::set<String> defaultImportSegments = {};
        bool merging = false;
    };
}

#endif //_EOKAS_SEMA_ANALYZER_H_
