#include "./cpp-backend.h"

namespace eokas
{
    /*
    ============================================================================
    ==== output helpers
    ============================================================================
    */
    String cpp_backend_t::indent() const
    {
        return String(' ', (size_t) m_indent * 4);
    }

    void cpp_backend_t::line(const String& s)
    {
        if (s.isEmpty())
        {
            m_out << "\n";
            return;
        }
        m_out << this->indent().cstr() << s.cstr() << "\n";
    }

    void cpp_backend_t::raw(const String& s)
    {
        m_out << s.cstr();
    }

    /*
    ============================================================================
    ==== entry
    ============================================================================
    */
    String cpp_backend_t::generate(sema_module_t* module)
    {
        m_out.str("");
        m_out.clear();
        m_indent = 0;
        m_error.clear();
        m_module = module;

        if (module == nullptr)
        {
            m_error = "module is null.";
            return "";
        }

        this->line(String::format("// Generated from eokas module: %s", module->get_name().cstr()));
        this->line("// Target standard: C++20 (sema-driven backend)");
        this->line("#include <cstdint>");
        this->line("#include <string>");
        this->line("#include <vector>");
        this->line("#include <functional>");
        this->line("#include <utility>");
        this->line("#include <iostream>");
        this->line("");

        for (sema_decl_t* decl : module->toplevels())
        {
            if (!this->gen_decl(decl))
                return "";
        }

        return String(m_out.str());
    }

    /*
    ============================================================================
    ==== declarations
    ============================================================================
    */
    bool cpp_backend_t::gen_decl(sema_decl_t* decl)
    {
        if (decl == nullptr)
            return true;

        switch (decl->kind)
        {
            case sema_decl_kind_t::STRUCT:
                return this->gen_struct(static_cast<sema_type_struct_t*>(decl->type));
            case sema_decl_kind_t::ENUM:
                return this->gen_enum(static_cast<sema_type_enum_t*>(decl->type));
            case sema_decl_kind_t::SCHEMA:
                return this->gen_schema(static_cast<sema_type_schema_t*>(decl->type));
            case sema_decl_kind_t::META:
                return this->gen_meta(decl->meta);
            case sema_decl_kind_t::FUNC:
                return this->gen_func(decl->func);
            case sema_decl_kind_t::SYMBOL:
                return this->gen_global(decl->symbol);
            default:
                return true;
        }
    }

    bool cpp_backend_t::gen_struct(sema_type_struct_t* type)
    {
        if (type == nullptr)
            return false;

        if (!type->typeParams.empty())
        {
            String tps;
            for (size_t i = 0; i < type->typeParams.size(); i++)
            {
                if (i > 0) tps += ", ";
                tps += "typename ";
                tps += type->typeParams[i];
            }
            this->line(String::format("template<%s>", tps.cstr()));
        }

        String head = String::format("struct %s", type->get_name().cstr());
        if (!type->schemas.empty())
        {
            head += " : ";
            for (size_t i = 0; i < type->schemas.size(); i++)
            {
                if (i > 0) head += ", ";
                head += "public ";
                head += type->schemas[i]->get_name();
            }
        }
        head += " {";
        this->line(head);
        m_indent++;

        for (auto& f : type->fields)
        {
            String decl;
            if (f.isConst)
                decl += "const ";
            decl += this->gen_type(f.type);
            decl += " ";
            decl += f.name;
            if (f.value != nullptr)
            {
                decl += " = ";
                decl += this->gen_expr(f.value);
            }
            decl += ";";
            this->line(decl);
        }

        for (auto* method : type->methods)
        {
            if (method == nullptr)
                continue;
            this->line(this->gen_func_signature(method->name, method));
            this->gen_func_body(method);
        }

        m_indent--;
        this->line("};");
        this->line("");
        return true;
    }

    bool cpp_backend_t::gen_enum(sema_type_enum_t* type)
    {
        if (type == nullptr)
            return false;

        this->line(String::format("enum class %s {", type->get_name().cstr()));
        m_indent++;
        for (auto& m : type->members)
        {
            this->line(String::format("%s = %d,", m.first.cstr(), (int) m.second));
        }
        m_indent--;
        this->line("};");
        this->line("");
        return true;
    }

    bool cpp_backend_t::gen_schema(sema_type_schema_t* type)
    {
        if (type == nullptr)
            return false;

        String tparam = type->typeParams.empty() ? String("T") : type->typeParams[0];

        this->line(String::format("// schema %s -> C++20 concept (best-effort)", type->get_name().cstr()));
        this->line(String::format("template<typename %s>", tparam.cstr()));
        this->line(String::format("concept %s = requires(%s self) {", type->get_name().cstr(), tparam.cstr()));
        m_indent++;
        for (auto& m : type->members)
        {
            if (!m.isFunc)
                continue;
            this->line(String::format("{ self.%s };", m.name.cstr()));
        }
        m_indent--;
        this->line("};");
        this->line("");
        return true;
    }

    bool cpp_backend_t::gen_meta(sema_meta_t* meta)
    {
        if (meta == nullptr)
            return false;

        this->line(String::format("// meta %s", meta->name.cstr()));
        this->line(String::format("struct %s {", meta->name.cstr()));
        m_indent++;
        for (auto& f : meta->fields)
        {
            String decl = this->gen_type(f.type);
            decl += " ";
            decl += f.name;
            if (f.value != nullptr)
            {
                decl += " = ";
                decl += this->gen_expr(f.value);
            }
            decl += ";";
            this->line(decl);
        }
        m_indent--;
        this->line("};");
        this->line("");
        return true;
    }

    bool cpp_backend_t::gen_global(sema_stmt_symbol_def_t* sym)
    {
        if (sym == nullptr)
            return false;

        String decl;
        if (!sym->mutability)
            decl += "const ";
        decl += this->gen_type(sym->type);
        decl += " ";
        decl += sym->name;
        if (sym->value != nullptr)
        {
            decl += " = ";
            decl += this->gen_expr(sym->value);
        }
        decl += ";";
        this->line(decl);
        this->line("");
        return true;
    }

    bool cpp_backend_t::gen_func(sema_func_t* func)
    {
        if (func == nullptr)
            return false;

        if (!func->typeParams.empty())
        {
            String tps;
            for (size_t i = 0; i < func->typeParams.size(); i++)
            {
                if (i > 0) tps += ", ";
                tps += "typename ";
                tps += func->typeParams[i];
            }
            this->line(String::format("template<%s>", tps.cstr()));
        }

        if (func->isMain)
        {
            this->line("int main(int argc, char** argv) {");
            m_indent++;
            for (auto* stmt : func->body)
            {
                if (!this->gen_stmt(stmt))
                    return false;
            }
            m_indent--;
            this->line("}");
            this->line("");
            return true;
        }

        this->line(this->gen_func_signature(func->name, func));
        if (!this->gen_func_body(func))
            return false;
        this->line("");
        return true;
    }

    /*
    ============================================================================
    ==== types
    ============================================================================
    */
    String cpp_backend_t::gen_type(sema_type_t* type)
    {
        if (type == nullptr)
            return "void";

        switch (type->get_kind())
        {
            case sema_type_kind_t::PRIMITIVE:
            {
                auto p = static_cast<sema_type_primitive_t*>(type)->prim;
                switch (p)
                {
                    case sema_primitive_kind_t::VOID: return "void";
                    case sema_primitive_kind_t::BOOL: return "bool";
                    case sema_primitive_kind_t::STRING: return "std::string";
                    case sema_primitive_kind_t::I8: return "std::int8_t";
                    case sema_primitive_kind_t::I16: return "std::int16_t";
                    case sema_primitive_kind_t::I32: return "std::int32_t";
                    case sema_primitive_kind_t::I64: return "std::int64_t";
                    case sema_primitive_kind_t::U8: return "std::uint8_t";
                    case sema_primitive_kind_t::U16: return "std::uint16_t";
                    case sema_primitive_kind_t::U32: return "std::uint32_t";
                    case sema_primitive_kind_t::U64: return "std::uint64_t";
                    case sema_primitive_kind_t::F32: return "float";
                    case sema_primitive_kind_t::F64: return "double";
                }
                return "void";
            }
            case sema_type_kind_t::FUNC:
            {
                auto f = static_cast<sema_type_func_t*>(type);
                String ret = this->gen_type(f->ret);
                String params;
                for (size_t i = 0; i < f->params.size(); i++)
                {
                    if (i > 0) params += ", ";
                    params += this->gen_type(f->params[i]);
                }
                return String::format("std::function<%s(%s)>", ret.cstr(), params.cstr());
            }
            case sema_type_kind_t::HEAP:
            {
                auto h = static_cast<sema_type_handle_t*>(type);
                return String::format("std::vector<%s>", this->gen_type(h->element).cstr());
            }
            case sema_type_kind_t::SLOT:
            {
                auto h = static_cast<sema_type_handle_t*>(type);
                return this->gen_type(h->element);
            }
            case sema_type_kind_t::STRUCT:
            {
                auto s = static_cast<sema_type_struct_t*>(type);
                if (s->typeArgs.empty())
                    return s->get_name();
                String r = s->get_name();
                r += "<";
                for (size_t i = 0; i < s->typeArgs.size(); i++)
                {
                    if (i > 0) r += ", ";
                    r += this->gen_type(s->typeArgs[i]);
                }
                r += ">";
                return r;
            }
            case sema_type_kind_t::ENUM:
            case sema_type_kind_t::GENERIC_PARAM:
            case sema_type_kind_t::SCHEMA:
                return type->get_name();
            case sema_type_kind_t::ERROR_TYPE:
                return "/*error*/ void";
            default:
                return "void";
        }
    }

    String cpp_backend_t::gen_func_signature(const String& name, sema_func_t* func)
    {
        String ret = this->gen_type(func->ret);
        String params;
        for (size_t i = 0; i < func->params.size(); i++)
        {
            if (i > 0) params += ", ";
            params += this->gen_type(func->params[i].type);
            params += " ";
            params += func->params[i].name;
        }
        return String::format("%s %s(%s) {", ret.cstr(), name.cstr(), params.cstr());
    }

    bool cpp_backend_t::gen_func_body(sema_func_t* func)
    {
        m_indent++;
        for (auto* stmt : func->body)
        {
            if (!this->gen_stmt(stmt))
                return false;
        }
        m_indent--;
        this->line("}");
        return true;
    }

    /*
    ============================================================================
    ==== statements
    ============================================================================
    */
    bool cpp_backend_t::gen_stmt(sema_stmt_t* stmt)
    {
        if (stmt == nullptr)
            return true;

        switch (stmt->kind)
        {
            case sema_stmt_kind_t::BLOCK: return this->gen_block(static_cast<sema_stmt_block_t*>(stmt));
            case sema_stmt_kind_t::SYMBOL_DEF: return this->gen_symbol_def(static_cast<sema_stmt_symbol_def_t*>(stmt));
            case sema_stmt_kind_t::ASSIGN: return this->gen_assign(static_cast<sema_stmt_assign_t*>(stmt));
            case sema_stmt_kind_t::RETURN: return this->gen_return(static_cast<sema_stmt_return_t*>(stmt));
            case sema_stmt_kind_t::IF: return this->gen_if(static_cast<sema_stmt_if_t*>(stmt));
            case sema_stmt_kind_t::FOR: return this->gen_for(static_cast<sema_stmt_for_t*>(stmt));
            case sema_stmt_kind_t::WHILE: return this->gen_while(static_cast<sema_stmt_while_t*>(stmt));
            case sema_stmt_kind_t::SWITCH: return this->gen_switch(static_cast<sema_stmt_switch_t*>(stmt));
            case sema_stmt_kind_t::BREAK: this->line("break;"); return true;
            case sema_stmt_kind_t::CONTINUE: this->line("continue;"); return true;
            case sema_stmt_kind_t::INVOKE: return this->gen_invoke(static_cast<sema_stmt_invoke_t*>(stmt));
            default: return true;
        }
    }

    bool cpp_backend_t::gen_block(sema_stmt_block_t* stmt)
    {
        this->line("{");
        m_indent++;
        for (auto* s : stmt->stmts)
        {
            if (!this->gen_stmt(s))
                return false;
        }
        m_indent--;
        this->line("}");
        return true;
    }

    bool cpp_backend_t::gen_symbol_def(sema_stmt_symbol_def_t* stmt)
    {
        this->line(this->gen_stmt_inline(stmt) + ";");
        return true;
    }

    bool cpp_backend_t::gen_assign(sema_stmt_assign_t* stmt)
    {
        this->line(this->gen_stmt_inline(stmt) + ";");
        return true;
    }

    bool cpp_backend_t::gen_return(sema_stmt_return_t* stmt)
    {
        if (stmt->value != nullptr)
            this->line(String::format("return %s;", this->gen_expr(stmt->value).cstr()));
        else
            this->line("return;");
        return true;
    }

    bool cpp_backend_t::gen_if(sema_stmt_if_t* stmt)
    {
        this->line(String::format("if (%s)", this->gen_expr(stmt->cond).cstr()));
        if (stmt->branch_true != nullptr)
        {
            bool brace = stmt->branch_true->kind == sema_stmt_kind_t::BLOCK;
            if (!brace) m_indent++;
            if (!this->gen_stmt(stmt->branch_true)) return false;
            if (!brace) m_indent--;
        }
        else
        {
            this->line("{}");
        }

        if (stmt->branch_false != nullptr)
        {
            this->line("else");
            bool brace = stmt->branch_false->kind == sema_stmt_kind_t::BLOCK;
            if (!brace) m_indent++;
            if (!this->gen_stmt(stmt->branch_false)) return false;
            if (!brace) m_indent--;
        }
        return true;
    }

    bool cpp_backend_t::gen_for(sema_stmt_for_t* stmt)
    {
        String init = stmt->init != nullptr ? this->gen_stmt_inline(stmt->init) : String("");
        String cond = stmt->cond != nullptr ? this->gen_expr(stmt->cond) : String("");
        String step = stmt->step != nullptr ? this->gen_stmt_inline(stmt->step) : String("");

        this->line(String::format("for (%s; %s; %s)", init.cstr(), cond.cstr(), step.cstr()));
        if (stmt->body != nullptr)
        {
            bool brace = stmt->body->kind == sema_stmt_kind_t::BLOCK;
            if (!brace) m_indent++;
            if (!this->gen_stmt(stmt->body)) return false;
            if (!brace) m_indent--;
        }
        else
        {
            this->line("{}");
        }
        return true;
    }

    bool cpp_backend_t::gen_while(sema_stmt_while_t* stmt)
    {
        this->line(String::format("while (%s)", this->gen_expr(stmt->cond).cstr()));
        if (stmt->body != nullptr)
        {
            bool brace = stmt->body->kind == sema_stmt_kind_t::BLOCK;
            if (!brace) m_indent++;
            if (!this->gen_stmt(stmt->body)) return false;
            if (!brace) m_indent--;
        }
        else
        {
            this->line("{}");
        }
        return true;
    }

    bool cpp_backend_t::gen_switch(sema_stmt_switch_t* stmt)
    {
        this->line(String::format("switch (%s) {", this->gen_expr(stmt->expr).cstr()));
        m_indent++;
        for (auto& c : stmt->cases)
        {
            this->line(String::format("case %s: {", this->gen_expr(c.value).cstr()));
            m_indent++;
            for (auto* s : c.body)
            {
                if (!this->gen_stmt(s)) return false;
            }
            this->line("break;");
            m_indent--;
            this->line("}");
        }
        if (!stmt->default_body.empty())
        {
            this->line("default: {");
            m_indent++;
            for (auto* s : stmt->default_body)
            {
                if (!this->gen_stmt(s)) return false;
            }
            this->line("break;");
            m_indent--;
            this->line("}");
        }
        m_indent--;
        this->line("}");
        return true;
    }

    bool cpp_backend_t::gen_invoke(sema_stmt_invoke_t* stmt)
    {
        if (stmt->expr == nullptr)
            return true;
        this->line(this->gen_func_ref(stmt->expr) + ";");
        return true;
    }

    String cpp_backend_t::gen_stmt_inline(sema_stmt_t* stmt)
    {
        if (stmt == nullptr)
            return "";

        switch (stmt->kind)
        {
            case sema_stmt_kind_t::SYMBOL_DEF:
            {
                auto* s = static_cast<sema_stmt_symbol_def_t*>(stmt);
                String decl;
                if (!s->mutability)
                    decl += "const ";
                decl += this->gen_type(s->type);
                decl += " ";
                decl += s->name;
                if (s->value != nullptr)
                {
                    decl += " = ";
                    decl += this->gen_expr(s->value);
                }
                return decl;
            }
            case sema_stmt_kind_t::ASSIGN:
            {
                auto* s = static_cast<sema_stmt_assign_t*>(stmt);
                return String::format("%s = %s", this->gen_expr(s->left).cstr(), this->gen_expr(s->right).cstr());
            }
            case sema_stmt_kind_t::INVOKE:
            {
                auto* s = static_cast<sema_stmt_invoke_t*>(stmt);
                return this->gen_func_ref(s->expr);
            }
            default:
                return "";
        }
    }

    /*
    ============================================================================
    ==== expressions
    ============================================================================
    */
    String cpp_backend_t::gen_expr(sema_expr_t* expr)
    {
        if (expr == nullptr)
            return "";

        switch (expr->kind)
        {
            case sema_expr_kind_t::TRINARY:
            {
                auto* n = static_cast<sema_expr_trinary_t*>(expr);
                return String::format("(%s ? %s : %s)",
                    this->gen_expr(n->cond).cstr(),
                    this->gen_expr(n->branch_true).cstr(),
                    this->gen_expr(n->branch_false).cstr());
            }
            case sema_expr_kind_t::BINARY:
            {
                auto* n = static_cast<sema_expr_binary_t*>(expr);
                if (n->dispatch == sema_oper_dispatch_t::SCHEMA_METHOD)
                {
                    return String::format("(%s).%s(%s)",
                        this->gen_expr(n->left).cstr(),
                        n->schemaMethod.cstr(),
                        this->gen_expr(n->right).cstr());
                }
                return String::format("(%s %s %s)",
                    this->gen_expr(n->left).cstr(),
                    map_binary(n->op).cstr(),
                    this->gen_expr(n->right).cstr());
            }
            case sema_expr_kind_t::UNARY:
            {
                auto* n = static_cast<sema_expr_unary_t*>(expr);
                if (n->dispatch == sema_oper_dispatch_t::SCHEMA_METHOD)
                {
                    return String::format("(%s).%s()",
                        this->gen_expr(n->operand).cstr(),
                        n->schemaMethod.cstr());
                }
                return String::format("(%s%s)",
                    map_unary(n->op).cstr(),
                    this->gen_expr(n->operand).cstr());
            }
            case sema_expr_kind_t::LITERAL_INT:
                return String::format("%lldLL", (long long) static_cast<sema_expr_literal_int_t*>(expr)->value);
            case sema_expr_kind_t::LITERAL_FLOAT:
                return String::valueToString(static_cast<sema_expr_literal_float_t*>(expr)->value);
            case sema_expr_kind_t::LITERAL_BOOL:
                return static_cast<sema_expr_literal_bool_t*>(expr)->value ? String("true") : String("false");
            case sema_expr_kind_t::LITERAL_STRING:
                return this->gen_literal_string(static_cast<sema_expr_literal_string_t*>(expr)->value);
            case sema_expr_kind_t::SYMBOL_REF:
            {
                auto* n = static_cast<sema_expr_symbol_ref_t*>(expr);
                if (n->name == "this")
                    return "(*this)";
                return n->name;
            }
            case sema_expr_kind_t::FUNC_REF:
                return this->gen_func_ref(static_cast<sema_expr_func_ref_t*>(expr));
            case sema_expr_kind_t::FUNC_DEF:
                return this->gen_func_lambda(static_cast<sema_expr_func_def_t*>(expr)->func);
            case sema_expr_kind_t::ARRAY_REF:
            {
                auto* n = static_cast<sema_expr_array_ref_t*>(expr);
                return String::format("%s[%s]",
                    this->gen_expr(n->obj).cstr(),
                    this->gen_expr(n->key).cstr());
            }
            case sema_expr_kind_t::OBJECT_DEF:
                return this->gen_object_def(static_cast<sema_expr_object_def_t*>(expr));
            case sema_expr_kind_t::OBJECT_REF:
            {
                auto* n = static_cast<sema_expr_object_ref_t*>(expr);
                // enum member access -> EnumName::Variant
                if (n->obj != nullptr && n->obj->type != nullptr && n->obj->type->is_enum())
                    return String::format("%s::%s", n->obj->type->get_name().cstr(), n->key.cstr());
                return String::format("%s.%s", this->gen_expr(n->obj).cstr(), n->key.cstr());
            }
            default:
                return "/* unsupported expr */";
        }
    }

    String cpp_backend_t::gen_func_lambda(sema_func_t* func)
    {
        if (func == nullptr)
            return "";

        String params;
        for (size_t i = 0; i < func->params.size(); i++)
        {
            if (i > 0) params += ", ";
            params += this->gen_type(func->params[i].type);
            params += " ";
            params += func->params[i].name;
        }

        String ret = this->gen_type(func->ret);
        String head = String::format("[=](%s) -> %s {", params.cstr(), ret.cstr());

        std::stringstream scratch;
        m_out.swap(scratch);
        m_out << head.cstr() << "\n";
        m_indent++;
        for (auto* stmt : func->body)
        {
            this->gen_stmt(stmt);
        }
        m_indent--;
        m_out << this->indent().cstr() << "}";

        String lambda(m_out.str());
        m_out.swap(scratch);
        return lambda;
    }

    String cpp_backend_t::gen_func_ref(sema_expr_func_ref_t* expr)
    {
        if (expr == nullptr)
            return "";

        String callee = this->gen_expr(expr->callee);

        String targs;
        if (!expr->typeArgs.empty())
        {
            targs += "<";
            for (size_t i = 0; i < expr->typeArgs.size(); i++)
            {
                if (i > 0) targs += ", ";
                targs += this->gen_type(expr->typeArgs[i]);
            }
            targs += ">";
        }

        String args;
        for (size_t i = 0; i < expr->args.size(); i++)
        {
            if (i > 0) args += ", ";
            args += this->gen_expr(expr->args[i]);
        }

        return String::format("%s%s(%s)", callee.cstr(), targs.cstr(), args.cstr());
    }

    String cpp_backend_t::gen_object_def(sema_expr_object_def_t* expr)
    {
        if (expr == nullptr)
            return "";

        String type = this->gen_type(expr->type);

        String fields;
        for (size_t i = 0; i < expr->members.size(); i++)
        {
            if (i > 0) fields += ", ";
            fields += String::format(".%s = %s", expr->members[i].first.cstr(), this->gen_expr(expr->members[i].second).cstr());
        }

        return String::format("%s{%s}", type.cstr(), fields.cstr());
    }

    String cpp_backend_t::gen_literal_string(const String& raw)
    {
        String escaped;
        for (size_t i = 0; i < raw.length(); i++)
        {
            char c = raw.at(i);
            switch (c)
            {
                case '\\': escaped += "\\\\"; break;
                case '\"': escaped += "\\\""; break;
                case '\n': escaped += "\\n"; break;
                case '\r': escaped += "\\r"; break;
                case '\t': escaped += "\\t"; break;
                default: escaped += String(c, 1); break;
            }
        }
        return String::format("std::string(\"%s\")", escaped.cstr());
    }

    /*
    ============================================================================
    ==== operators
    ============================================================================
    */
    String cpp_backend_t::map_binary(ast_binary_oper_t op)
    {
        switch (op)
        {
            case ast_binary_oper_t::OR: return "||";
            case ast_binary_oper_t::AND: return "&&";
            case ast_binary_oper_t::EQ: return "==";
            case ast_binary_oper_t::NE: return "!=";
            case ast_binary_oper_t::LE: return "<=";
            case ast_binary_oper_t::GE: return ">=";
            case ast_binary_oper_t::LT: return "<";
            case ast_binary_oper_t::GT: return ">";
            case ast_binary_oper_t::ADD: return "+";
            case ast_binary_oper_t::SUB: return "-";
            case ast_binary_oper_t::MUL: return "*";
            case ast_binary_oper_t::DIV: return "/";
            case ast_binary_oper_t::MOD: return "%";
            case ast_binary_oper_t::BIT_AND: return "&";
            case ast_binary_oper_t::BIT_OR: return "|";
            case ast_binary_oper_t::BIT_XOR: return "^";
            case ast_binary_oper_t::SHIFT_L: return "<<";
            case ast_binary_oper_t::SHIFT_R: return ">>";
            default: return "/*?op*/";
        }
    }

    String cpp_backend_t::map_unary(ast_unary_oper_t op)
    {
        switch (op)
        {
            case ast_unary_oper_t::POS: return "+";
            case ast_unary_oper_t::NEG: return "-";
            case ast_unary_oper_t::FLIP: return "~";
            case ast_unary_oper_t::NOT: return "!";
            default: return "/*?op*/";
        }
    }
}
