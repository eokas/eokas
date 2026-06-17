#ifndef _EOKAS_SEMA_BUILTINS_H_
#define _EOKAS_SEMA_BUILTINS_H_

#include "sema-header.h"
#include "sema-type.h"
#include "sema-symbol.h"

namespace eokas
{
    /**
     * Owns the shared kernel builtins that are implicitly visible to every
     * module: the operator capability Schemas (Add, Sub, Equals, ...) and the
     * pre-imported kernel functions (make / drop / is_valid / space_of).
     *
     * Primitive types live in `sema_type_registry_t`; this class injects the
     * primitive type names plus the schema names and kernel functions into a
     * module's root scope.
     */
    class sema_builtins_t
    {
    public:
        explicit sema_builtins_t(sema_type_registry_t* registry);
        ~sema_builtins_t();

        // Injects builtin type and value names into the given module root scope.
        void inject(sema_module_t* module);

        sema_type_schema_t* get_schema(const String& name) const;

        // The names of the operator schemas, keyed by operator.
        static String binary_op_schema(ast_binary_oper_t op);
        static String binary_op_method(ast_binary_oper_t op);
        static String unary_op_schema(ast_unary_oper_t op);
        static String unary_op_method(ast_unary_oper_t op);

    private:
        sema_type_schema_t* declare_schema(const String& name, const std::vector<String>& typeParams);

    private:
        sema_type_registry_t* registry;
        std::map<String, sema_type_schema_t*> schemas = {};
        std::vector<sema_type_t*> ownedTypes = {};
        std::vector<sema_value_symbol_t*> values = {};
        std::vector<sema_type_symbol_t*> types = {};
    };
}

#endif //_EOKAS_SEMA_BUILTINS_H_
