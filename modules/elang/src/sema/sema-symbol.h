#ifndef _EOKAS_SEMA_SYMBOL_H_
#define _EOKAS_SEMA_SYMBOL_H_

#include "sema-header.h"

namespace eokas
{
    /**
     * A value symbol: a named binding to a value of a given type (variable,
     * constant, function, parameter, imported value, ...).
     */
    struct sema_value_symbol_t
    {
        String name = "";
        sema_type_t* type = nullptr;
        bool mutability = false;     // true for `var`, false for `val`
        bool exported = false;
        bool isBuiltin = false;      // kernel pre-imported value (e.g. make / drop)
        ast_node_t* decl = nullptr;  // originating AST declaration (optional)
        sema_scope_t* scope = nullptr;
    };

    /**
     * A type symbol: a named binding to a type (struct / enum / schema / type
     * parameter / imported type).
     */
    struct sema_type_symbol_t
    {
        String name = "";
        sema_type_t* type = nullptr;
        bool exported = false;
        bool isBuiltin = false;
    };
}

#endif //_EOKAS_SEMA_SYMBOL_H_
