#include "sema-builtins.h"
#include "sema-module.h"

namespace eokas
{
    sema_builtins_t::sema_builtins_t(sema_type_registry_t* registry)
        : registry(registry)
    {
        // Operator capability schemas (eokas.kernel). Generic over <T> (or
        // <Index, Item> / <T, C>); members are not needed for code generation,
        // only the schema identity is used for operator dispatch & constraints.
        declare_schema("Add", {"T"});
        declare_schema("Sub", {"T"});
        declare_schema("Mul", {"T"});
        declare_schema("Div", {"T"});
        declare_schema("Mod", {"T"});
        declare_schema("Neg", {"T"});
        declare_schema("Equals", {"T"});
        declare_schema("Compare", {"T"});
        declare_schema("Assign", {"T"});
        declare_schema("Predicate", {"T"});
        declare_schema("BitOp", {"T"});
        declare_schema("IndexOp", {"Index", "Item"});
        declare_schema("Enumerable", {"T", "C"});
    }

    sema_builtins_t::~sema_builtins_t()
    {
        _DeleteList(values);
        _DeleteList(types);
        _DeleteList(ownedTypes);
        schemas.clear();
    }

    sema_type_schema_t* sema_builtins_t::declare_schema(const String& name, const std::vector<String>& typeParams)
    {
        auto* s = new sema_type_schema_t(name);
        s->typeParams = typeParams;
        ownedTypes.push_back(s);
        schemas.insert(std::make_pair(name, s));
        return s;
    }

    sema_type_schema_t* sema_builtins_t::get_schema(const String& name) const
    {
        auto iter = schemas.find(name);
        return iter != schemas.end() ? iter->second : nullptr;
    }

    void sema_builtins_t::inject(sema_module_t* module)
    {
        sema_scope_t* root = module->get_root();

        // primitive type names
        const char* prims[] = {
            "void", "bool", "String",
            "i8", "i16", "i32", "i64",
            "u8", "u16", "u32", "u64",
            "f32", "f64",
        };
        for (const char* p : prims)
        {
            String name(p);
            auto* sym = new sema_type_symbol_t();
            sym->name = name;
            sym->type = registry->get_primitive(name);
            sym->isBuiltin = true;
            types.push_back(sym);
            root->add_type(sym);
        }

        // schema names
        for (auto& pair : schemas)
        {
            auto* sym = new sema_type_symbol_t();
            sym->name = pair.first;
            sym->type = pair.second;
            sym->isBuiltin = true;
            types.push_back(sym);
            root->add_type(sym);
        }

        // kernel functions (make / drop / is_valid / space_of). Their generic
        // signatures are handled specially by the analyzer; here they are simply
        // visible builtin value names.
        const char* fns[] = { "make", "drop", "is_valid", "space_of" };
        for (const char* f : fns)
        {
            auto* sym = new sema_value_symbol_t();
            sym->name = String(f);
            sym->type = nullptr; // resolved per call-site
            sym->isBuiltin = true;
            values.push_back(sym);
            root->add_value(sym);
        }
    }

    String sema_builtins_t::binary_op_schema(ast_binary_oper_t op)
    {
        switch (op)
        {
            case ast_binary_oper_t::ADD: return "Add";
            case ast_binary_oper_t::SUB: return "Sub";
            case ast_binary_oper_t::MUL: return "Mul";
            case ast_binary_oper_t::DIV: return "Div";
            case ast_binary_oper_t::MOD: return "Mod";
            case ast_binary_oper_t::EQ:
            case ast_binary_oper_t::NE: return "Equals";
            case ast_binary_oper_t::LT:
            case ast_binary_oper_t::GT:
            case ast_binary_oper_t::LE:
            case ast_binary_oper_t::GE: return "Compare";
            case ast_binary_oper_t::AND:
            case ast_binary_oper_t::OR: return "Predicate";
            case ast_binary_oper_t::BIT_AND:
            case ast_binary_oper_t::BIT_OR:
            case ast_binary_oper_t::BIT_XOR:
            case ast_binary_oper_t::SHIFT_L:
            case ast_binary_oper_t::SHIFT_R: return "BitOp";
            default: return "";
        }
    }

    String sema_builtins_t::binary_op_method(ast_binary_oper_t op)
    {
        switch (op)
        {
            case ast_binary_oper_t::ADD: return "add";
            case ast_binary_oper_t::SUB: return "sub";
            case ast_binary_oper_t::MUL: return "mul";
            case ast_binary_oper_t::DIV: return "div";
            case ast_binary_oper_t::MOD: return "mod";
            case ast_binary_oper_t::EQ: return "equals";
            case ast_binary_oper_t::NE: return "equals";
            case ast_binary_oper_t::LT:
            case ast_binary_oper_t::GT:
            case ast_binary_oper_t::LE:
            case ast_binary_oper_t::GE: return "compare";
            case ast_binary_oper_t::AND: return "and";
            case ast_binary_oper_t::OR: return "or";
            case ast_binary_oper_t::BIT_AND: return "bit_and";
            case ast_binary_oper_t::BIT_OR: return "bit_or";
            case ast_binary_oper_t::BIT_XOR: return "bit_xor";
            case ast_binary_oper_t::SHIFT_L: return "bit_shl";
            case ast_binary_oper_t::SHIFT_R: return "bit_shr";
            default: return "";
        }
    }

    String sema_builtins_t::unary_op_schema(ast_unary_oper_t op)
    {
        switch (op)
        {
            case ast_unary_oper_t::NEG: return "Neg";
            case ast_unary_oper_t::NOT: return "Predicate";
            case ast_unary_oper_t::FLIP: return "BitOp";
            default: return "";
        }
    }

    String sema_builtins_t::unary_op_method(ast_unary_oper_t op)
    {
        switch (op)
        {
            case ast_unary_oper_t::NEG: return "neg";
            case ast_unary_oper_t::NOT: return "not";
            case ast_unary_oper_t::FLIP: return "bit_flip";
            default: return "";
        }
    }
}
