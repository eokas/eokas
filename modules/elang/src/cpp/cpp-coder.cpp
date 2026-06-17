
#include "./cpp-coder.h"

namespace eokas
{
    /*
    ============================================================================
    ==== output helpers
    ============================================================================
    */
    String cpp_coder_t::indent() const
    {
        return String(' ', (size_t) m_indent * 4);
    }

    void cpp_coder_t::line(const String& s)
    {
        if (s.isEmpty())
        {
            m_out << "\n";
            return;
        }
        m_out << this->indent().cstr() << s.cstr() << "\n";
    }

    void cpp_coder_t::raw(const String& s)
    {
        m_out << s.cstr();
    }

    /*
    ============================================================================
    ==== entry
    ============================================================================
    */
    String cpp_coder_t::generate(ast_node_module_t* node)
    {
        m_out.str("");
        m_out.clear();
        m_indent = 0;
        m_error.clear();

        if (node == nullptr)
        {
            m_error = "module node is null.";
            return "";
        }

        // File header.
        this->line(String::format("// Generated from eokas module: %s", node->name.isEmpty() ? "<main>" : node->name.cstr()));
        this->line("// Target standard: C++20");
        this->line("#include <cstdint>");
        this->line("#include <string>");
        this->line("#include <vector>");
        this->line("#include <functional>");
        this->line("#include <utility>");
        this->line("#include <iostream>");
        this->line("");

        if (node->entry == nullptr)
        {
            m_error = "module entry is null.";
            return "";
        }

        // The parser stores all top-level declarations inside entry->body.
        for (ast_node_stmt_t* stmt : node->entry->body)
        {
            if (!this->gen_toplevel(stmt))
                return "";
        }

        return String(m_out.str());
    }

    /*
    ============================================================================
    ==== top-level
    ============================================================================
    */
    bool cpp_coder_t::gen_toplevel(ast_node_t* node)
    {
        if (node == nullptr)
        {
            m_error = "top-level node is null.";
            return false;
        }

        switch (node->category)
        {
            case ast_category_t::STRUCT_DEF:
                return this->gen_struct(reinterpret_cast<ast_node_struct_def_t*>(node));
            case ast_category_t::ENUM_DEF:
                return this->gen_enum(reinterpret_cast<ast_node_enum_def_t*>(node));
            case ast_category_t::SCHEMA_DEF:
                return this->gen_schema(reinterpret_cast<ast_node_schema_def_t*>(node));
            case ast_category_t::META_DEF:
                return this->gen_meta(reinterpret_cast<ast_node_meta_def_t*>(node));
            case ast_category_t::FUNC_DEF:
            {
                auto* fn = reinterpret_cast<ast_node_func_def_t*>(node);
                return this->gen_named_func(fn->name, fn, fn->name == "main");
            }
            case ast_category_t::SYMBOL_DEF:
            {
                auto* sym = reinterpret_cast<ast_node_symbol_def_t*>(node);
                if (sym->value != nullptr && sym->value->category == ast_category_t::FUNC_DEF)
                {
                    auto* fn = reinterpret_cast<ast_node_func_def_t*>(sym->value);
                    return this->gen_named_func(sym->name, fn, sym->name == "main");
                }
                return this->gen_global_symbol(sym);
            }
            default:
                // Any other free-standing top-level statement is unusual; emit
                // it verbatim as a best-effort global statement comment.
                this->line(String::format("// [cpp-coder] skipped unsupported top-level node (category=%d)", (int) node->category));
                return true;
        }
    }

    bool cpp_coder_t::gen_struct(ast_node_struct_def_t* node)
    {
        if (node == nullptr)
            return false;

        if (!node->typeParams.empty())
        {
            String tps;
            for (size_t i = 0; i < node->typeParams.size(); i++)
            {
                if (i > 0) tps += ", ";
                tps += "typename ";
                tps += node->typeParams[i];
            }
            this->line(String::format("template<%s>", tps.cstr()));
        }

        String head = String::format("struct %s", node->name.cstr());
        if (!node->schemas.empty())
        {
            head += " : ";
            for (size_t i = 0; i < node->schemas.size(); i++)
            {
                if (i > 0) head += ", ";
                head += "public ";
                head += this->gen_type(node->schemas[i]);
            }
        }
        head += " {";
        this->line(head);
        m_indent++;

        for (auto& m : node->members)
        {
            String type = m.type != nullptr ? this->gen_type(m.type) : "auto";
            String decl;
            if (m.isConst)
                decl += "const ";
            decl += type;
            decl += " ";
            decl += m.name;
            if (m.value != nullptr)
            {
                decl += " = ";
                decl += this->gen_expr(m.value);
            }
            decl += ";";
            this->line(decl);
        }

        for (auto* method : node->methods)
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

    bool cpp_coder_t::gen_enum(ast_node_enum_def_t* node)
    {
        if (node == nullptr)
            return false;

        this->line(String::format("enum class %s {", node->name.cstr()));
        m_indent++;
        for (auto& m : node->members)
        {
            this->line(String::format("%s = %d,", m.first.cstr(), m.second));
        }
        m_indent--;
        this->line("};");
        this->line("");
        return true;
    }

    bool cpp_coder_t::gen_schema(ast_node_schema_def_t* node)
    {
        if (node == nullptr)
            return false;

        // Eokas schema is a compile-time contract -> map to a C++20 concept
        // (best-effort). The type parameter defaults to `T` when the schema
        // declares none.
        String tparam = node->typeParams.empty() ? String("T") : node->typeParams[0];

        this->line(String::format("// schema %s -> C++20 concept (best-effort)", node->name.cstr()));
        this->line(String::format("template<typename %s>", tparam.cstr()));
        this->line(String::format("concept %s = requires(%s self) {", node->name.cstr(), tparam.cstr()));
        m_indent++;
        for (auto* method : node->methods)
        {
            if (method == nullptr)
                continue;
            String call = String::format("self.%s(", method->name.cstr());
            for (size_t i = 0; i < method->args.size(); i++)
            {
                if (i > 0) call += ", ";
                call += String::format("std::declval<%s>()",
                    method->args[i].type != nullptr ? this->gen_type(method->args[i].type).cstr() : "auto");
            }
            call += ")";
            this->line(String::format("{ %s };", call.cstr()));
        }
        m_indent--;
        this->line("};");
        this->line("");
        return true;
    }

    bool cpp_coder_t::gen_meta(ast_node_meta_def_t* node)
    {
        if (node == nullptr)
            return false;

        // Meta is compile-time metadata; emit as a plain struct of fields.
        this->line(String::format("// meta %s", node->name.cstr()));
        this->line(String::format("struct %s {", node->name.cstr()));
        m_indent++;
        for (auto& f : node->fields)
        {
            String type = f.type != nullptr ? this->gen_type(f.type) : "auto";
            String decl = type;
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

    bool cpp_coder_t::gen_global_symbol(ast_node_symbol_def_t* node)
    {
        if (node == nullptr)
            return false;

        String type = node->type != nullptr ? this->gen_type(node->type) : "auto";
        String decl;
        if (!node->variable)
            decl += "const ";
        decl += type;
        decl += " ";
        decl += node->name;
        if (node->value != nullptr)
        {
            decl += " = ";
            decl += this->gen_expr(node->value);
        }
        decl += ";";
        this->line(decl);
        this->line("");
        return true;
    }

    bool cpp_coder_t::gen_named_func(const String& name, ast_node_func_def_t* fn, bool asMain)
    {
        if (fn == nullptr)
            return false;

        if (asMain)
        {
            this->line("int main(int argc, char** argv) {");
            m_indent++;
            for (auto* stmt : fn->body)
            {
                if (!this->gen_stmt(stmt))
                    return false;
            }
            m_indent--;
            this->line("}");
            this->line("");
            return true;
        }

        this->line(this->gen_func_signature(name, fn));
        if (!this->gen_func_body(fn))
            return false;
        this->line("");
        return true;
    }

    /*
    ============================================================================
    ==== types and operators
    ============================================================================
    */
    String cpp_coder_t::gen_type(ast_node_type_t* node)
    {
        if (node == nullptr)
            return "void";

        const String& name = node->name;

        if (name == "func")
            return this->gen_func_type(node);

        if (name == "i8") return "std::int8_t";
        if (name == "i16") return "std::int16_t";
        if (name == "i32") return "std::int32_t";
        if (name == "i64") return "std::int64_t";
        if (name == "u8") return "std::uint8_t";
        if (name == "u16") return "std::uint16_t";
        if (name == "u32") return "std::uint32_t";
        if (name == "u64") return "std::uint64_t";
        if (name == "f32") return "float";
        if (name == "f64") return "double";
        if (name == "bool") return "bool";
        if (name == "void") return "void";
        if (name == "string" || name == "String") return "std::string";

        if (name == "array")
        {
            String inner = node->args.empty() ? String("void") : this->gen_type(node->args[0]);
            return String::format("std::vector<%s>", inner.cstr());
        }

        // User-defined type, possibly generic.
        if (node->args.empty())
            return name;

        String result = name;
        result += "<";
        for (size_t i = 0; i < node->args.size(); i++)
        {
            if (i > 0) result += ", ";
            result += this->gen_type(node->args[i]);
        }
        result += ">";
        return result;
    }

    String cpp_coder_t::gen_func_type(ast_node_type_t* node)
    {
        // node->args = [param types..., return type]
        if (node->args.empty())
            return "std::function<void()>";

        size_t count = node->args.size();
        String ret = this->gen_type(node->args[count - 1]);

        String params;
        for (size_t i = 0; i + 1 < count; i++)
        {
            if (i > 0) params += ", ";
            params += this->gen_type(node->args[i]);
        }
        return String::format("std::function<%s(%s)>", ret.cstr(), params.cstr());
    }

    String cpp_coder_t::map_binary(ast_binary_oper_t op)
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

    String cpp_coder_t::map_unary(ast_unary_oper_t op)
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

    /*
    ============================================================================
    ==== function helpers
    ============================================================================
    */
    String cpp_coder_t::gen_func_signature(const String& name, ast_node_func_def_t* fn)
    {
        String ret = fn->rtype != nullptr ? this->gen_type(fn->rtype) : "void";

        String params;
        for (size_t i = 0; i < fn->args.size(); i++)
        {
            if (i > 0) params += ", ";
            const auto& arg = fn->args[i];
            params += arg.type != nullptr ? this->gen_type(arg.type) : String("auto");
            params += " ";
            params += arg.name;
        }

        return String::format("%s %s(%s) {", ret.cstr(), name.cstr(), params.cstr());
    }

    bool cpp_coder_t::gen_func_body(ast_node_func_def_t* fn)
    {
        m_indent++;
        for (auto* stmt : fn->body)
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
    bool cpp_coder_t::gen_stmt(ast_node_stmt_t* node)
    {
        if (node == nullptr)
            return false;

        switch (node->category)
        {
            case ast_category_t::BLOCK:
                return this->gen_block(reinterpret_cast<ast_node_block_t*>(node));
            case ast_category_t::SYMBOL_DEF:
                return this->gen_symbol_def(reinterpret_cast<ast_node_symbol_def_t*>(node));
            case ast_category_t::ASSIGN:
                return this->gen_assign(reinterpret_cast<ast_node_assign_t*>(node));
            case ast_category_t::RETURN:
                return this->gen_return(reinterpret_cast<ast_node_return_t*>(node));
            case ast_category_t::IF:
                return this->gen_if(reinterpret_cast<ast_node_if_t*>(node));
            case ast_category_t::FOR:
                return this->gen_for(reinterpret_cast<ast_node_for_t*>(node));
            case ast_category_t::WHILE:
                return this->gen_while(reinterpret_cast<ast_node_while_t*>(node));
            case ast_category_t::SWITCH:
                return this->gen_switch(reinterpret_cast<ast_node_switch_t*>(node));
            case ast_category_t::BREAK:
                this->line("break;");
                return true;
            case ast_category_t::CONTINUE:
                this->line("continue;");
                return true;
            case ast_category_t::INVOKE:
                return this->gen_invoke(reinterpret_cast<ast_node_invoke_t*>(node));
            // Nested top-level-like declarations may appear within a body.
            case ast_category_t::STRUCT_DEF:
                return this->gen_struct(reinterpret_cast<ast_node_struct_def_t*>(node));
            case ast_category_t::ENUM_DEF:
                return this->gen_enum(reinterpret_cast<ast_node_enum_def_t*>(node));
            default:
                this->line(String::format("// [cpp-coder] skipped unsupported statement (category=%d)", (int) node->category));
                return true;
        }
    }

    bool cpp_coder_t::gen_block(ast_node_block_t* node)
    {
        if (node == nullptr)
            return false;

        this->line("{");
        m_indent++;
        for (auto* stmt : node->stmts)
        {
            if (!this->gen_stmt(stmt))
                return false;
        }
        m_indent--;
        this->line("}");
        return true;
    }

    bool cpp_coder_t::gen_symbol_def(ast_node_symbol_def_t* node)
    {
        if (node == nullptr)
            return false;
        this->line(this->gen_stmt_inline(node) + ";");
        return true;
    }

    bool cpp_coder_t::gen_assign(ast_node_assign_t* node)
    {
        if (node == nullptr)
            return false;
        this->line(this->gen_stmt_inline(node) + ";");
        return true;
    }

    bool cpp_coder_t::gen_return(ast_node_return_t* node)
    {
        if (node == nullptr)
            return false;
        if (node->value != nullptr)
            this->line(String::format("return %s;", this->gen_expr(node->value).cstr()));
        else
            this->line("return;");
        return true;
    }

    bool cpp_coder_t::gen_if(ast_node_if_t* node)
    {
        if (node == nullptr)
            return false;

        this->line(String::format("if (%s)", this->gen_expr(node->cond).cstr()));
        if (node->branch_true != nullptr)
        {
            if (node->branch_true->category != ast_category_t::BLOCK)
                m_indent++;
            if (!this->gen_stmt(node->branch_true))
                return false;
            if (node->branch_true->category != ast_category_t::BLOCK)
                m_indent--;
        }
        else
        {
            this->line("{}");
        }

        if (node->branch_false != nullptr)
        {
            this->line("else");
            if (node->branch_false->category != ast_category_t::BLOCK)
                m_indent++;
            if (!this->gen_stmt(node->branch_false))
                return false;
            if (node->branch_false->category != ast_category_t::BLOCK)
                m_indent--;
        }
        return true;
    }

    bool cpp_coder_t::gen_for(ast_node_for_t* node)
    {
        if (node == nullptr)
            return false;

        String init = node->init != nullptr ? this->gen_stmt_inline(node->init) : String("");
        String cond = node->cond != nullptr ? this->gen_expr(node->cond) : String("");
        String step = node->step != nullptr ? this->gen_stmt_inline(node->step) : String("");

        this->line(String::format("for (%s; %s; %s)", init.cstr(), cond.cstr(), step.cstr()));
        if (node->body != nullptr)
        {
            if (node->body->category != ast_category_t::BLOCK)
                m_indent++;
            if (!this->gen_stmt(node->body))
                return false;
            if (node->body->category != ast_category_t::BLOCK)
                m_indent--;
        }
        else
        {
            this->line("{}");
        }
        return true;
    }

    bool cpp_coder_t::gen_while(ast_node_while_t* node)
    {
        if (node == nullptr)
            return false;

        this->line(String::format("while (%s)", this->gen_expr(node->cond).cstr()));
        if (node->body != nullptr)
        {
            if (node->body->category != ast_category_t::BLOCK)
                m_indent++;
            if (!this->gen_stmt(node->body))
                return false;
            if (node->body->category != ast_category_t::BLOCK)
                m_indent--;
        }
        else
        {
            this->line("{}");
        }
        return true;
    }

    bool cpp_coder_t::gen_switch(ast_node_switch_t* node)
    {
        if (node == nullptr)
            return false;

        this->line(String::format("switch (%s) {", this->gen_expr(node->expr).cstr()));
        m_indent++;
        for (auto& c : node->cases)
        {
            this->line(String::format("case %s: {", this->gen_expr(c.value).cstr()));
            m_indent++;
            for (auto* stmt : c.body)
            {
                if (!this->gen_stmt(stmt))
                    return false;
            }
            this->line("break;");
            m_indent--;
            this->line("}");
        }
        if (!node->default_body.empty())
        {
            this->line("default: {");
            m_indent++;
            for (auto* stmt : node->default_body)
            {
                if (!this->gen_stmt(stmt))
                    return false;
            }
            this->line("break;");
            m_indent--;
            this->line("}");
        }
        m_indent--;
        this->line("}");
        return true;
    }

    bool cpp_coder_t::gen_invoke(ast_node_invoke_t* node)
    {
        if (node == nullptr || node->expr == nullptr)
            return false;
        this->line(this->gen_func_ref(node->expr) + ";");
        return true;
    }

    String cpp_coder_t::gen_stmt_inline(ast_node_stmt_t* node)
    {
        if (node == nullptr)
            return "";

        switch (node->category)
        {
            case ast_category_t::SYMBOL_DEF:
            {
                auto* sym = reinterpret_cast<ast_node_symbol_def_t*>(node);
                String type = sym->type != nullptr ? this->gen_type(sym->type) : String("auto");
                String decl;
                if (!sym->variable)
                    decl += "const ";
                decl += type;
                decl += " ";
                decl += sym->name;
                if (sym->value != nullptr)
                {
                    decl += " = ";
                    decl += this->gen_expr(sym->value);
                }
                return decl;
            }
            case ast_category_t::ASSIGN:
            {
                auto* a = reinterpret_cast<ast_node_assign_t*>(node);
                return String::format("%s = %s", this->gen_expr(a->left).cstr(), this->gen_expr(a->right).cstr());
            }
            case ast_category_t::INVOKE:
            {
                auto* inv = reinterpret_cast<ast_node_invoke_t*>(node);
                return this->gen_func_ref(inv->expr);
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
    String cpp_coder_t::gen_expr(ast_node_expr_t* node)
    {
        if (node == nullptr)
            return "";

        switch (node->category)
        {
            case ast_category_t::EXPR_TRINARY:
            {
                auto* n = reinterpret_cast<ast_node_expr_trinary_t*>(node);
                return String::format("(%s ? %s : %s)",
                    this->gen_expr(n->cond).cstr(),
                    this->gen_expr(n->branch_true).cstr(),
                    this->gen_expr(n->branch_false).cstr());
            }
            case ast_category_t::EXPR_BINARY:
            {
                auto* n = reinterpret_cast<ast_node_expr_binary_t*>(node);
                return String::format("(%s %s %s)",
                    this->gen_expr(n->left).cstr(),
                    this->map_binary(n->op).cstr(),
                    this->gen_expr(n->right).cstr());
            }
            case ast_category_t::EXPR_UNARY:
            {
                auto* n = reinterpret_cast<ast_node_expr_unary_t*>(node);
                return String::format("(%s%s)",
                    this->map_unary(n->op).cstr(),
                    this->gen_expr(n->right).cstr());
            }
            case ast_category_t::LITERAL_INT:
            {
                auto* n = reinterpret_cast<ast_node_literal_int_t*>(node);
                return String::format("%lldLL", (long long) n->value);
            }
            case ast_category_t::LITERAL_FLOAT:
            {
                auto* n = reinterpret_cast<ast_node_literal_float_t*>(node);
                return String::valueToString(n->value);
            }
            case ast_category_t::LITERAL_BOOL:
            {
                auto* n = reinterpret_cast<ast_node_literal_bool_t*>(node);
                return n->value ? String("true") : String("false");
            }
            case ast_category_t::LITERAL_STRING:
            {
                auto* n = reinterpret_cast<ast_node_literal_string_t*>(node);
                return this->gen_literal_string(n->value);
            }
            case ast_category_t::SYMBOL_REF:
            {
                auto* n = reinterpret_cast<ast_node_symbol_ref_t*>(node);
                return n->name;
            }
            case ast_category_t::FUNC_REF:
                return this->gen_func_ref(reinterpret_cast<ast_node_func_ref_t*>(node));
            case ast_category_t::FUNC_DEF:
                return this->gen_func_lambda(reinterpret_cast<ast_node_func_def_t*>(node));
            case ast_category_t::ARRAY_REF:
            {
                auto* n = reinterpret_cast<ast_node_array_ref_t*>(node);
                return String::format("%s[%s]",
                    this->gen_expr(n->obj).cstr(),
                    this->gen_expr(n->key).cstr());
            }
            case ast_category_t::OBJECT_DEF:
                return this->gen_object_def(reinterpret_cast<ast_node_object_def_t*>(node));
            case ast_category_t::OBJECT_REF:
            {
                auto* n = reinterpret_cast<ast_node_object_ref_t*>(node);
                return String::format("%s.%s",
                    this->gen_expr(n->obj).cstr(),
                    n->key.cstr());
            }
            default:
                return String::format("/* [cpp-coder] unsupported expr (category=%d) */", (int) node->category);
        }
    }

    String cpp_coder_t::gen_func_lambda(ast_node_func_def_t* node)
    {
        if (node == nullptr)
            return "";

        String params;
        for (size_t i = 0; i < node->args.size(); i++)
        {
            if (i > 0) params += ", ";
            const auto& arg = node->args[i];
            params += arg.type != nullptr ? this->gen_type(arg.type) : String("auto");
            params += " ";
            params += arg.name;
        }

        String ret = node->rtype != nullptr ? this->gen_type(node->rtype) : String("");

        String head = ret.isEmpty()
            ? String::format("[=](%s) {", params.cstr())
            : String::format("[=](%s) -> %s {", params.cstr(), ret.cstr());

        // Render the lambda into a temporary stream so the result can be
        // embedded inside an expression context (e.g. `auto f = <lambda>;`)
        // without disturbing the main output ordering. The first line carries
        // no indent (the caller positions it); nested body lines keep their
        // absolute indentation based on the current depth.
        std::stringstream scratch;
        m_out.swap(scratch); // m_out is now fresh; scratch holds prior output.

        m_out << head.cstr() << "\n";
        m_indent++;
        for (auto* stmt : node->body)
        {
            this->gen_stmt(stmt);
        }
        m_indent--;
        m_out << this->indent().cstr() << "}";

        String lambda(m_out.str());
        m_out.swap(scratch); // restore the main output stream.
        return lambda;
    }

    String cpp_coder_t::gen_func_ref(ast_node_func_ref_t* node)
    {
        if (node == nullptr)
            return "";

        String callee = this->gen_expr(node->func);

        String targs;
        if (!node->typeArgs.empty())
        {
            targs += "<";
            for (size_t i = 0; i < node->typeArgs.size(); i++)
            {
                if (i > 0) targs += ", ";
                targs += this->gen_type(node->typeArgs[i]);
            }
            targs += ">";
        }

        String args;
        for (size_t i = 0; i < node->args.size(); i++)
        {
            if (i > 0) args += ", ";
            args += this->gen_expr(node->args[i]);
        }

        return String::format("%s%s(%s)", callee.cstr(), targs.cstr(), args.cstr());
    }

    String cpp_coder_t::gen_object_def(ast_node_object_def_t* node)
    {
        if (node == nullptr)
            return "";

        String type = node->type != nullptr ? this->gen_type(node->type) : String("");

        String fields;
        size_t i = 0;
        for (auto& m : node->members)
        {
            if (i > 0) fields += ", ";
            fields += String::format(".%s = %s", m.first.cstr(), this->gen_expr(m.second).cstr());
            i++;
        }

        return String::format("%s{%s}", type.cstr(), fields.cstr());
    }

    String cpp_coder_t::gen_literal_string(const String& raw)
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
}
