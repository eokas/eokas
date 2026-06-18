#include "parser.h"
#include "scanner.h"
#include "../ast/ast.h"

namespace eokas {

	parser_t::parser_t()
		: scanner(new scanner_t())
		, factory(new ast_factory_t())
		, errormsg() { }

	parser_t::~parser_t() {
		this->clear();
		_DeletePointer(this->scanner);
		_DeletePointer(this->factory);
	}

	ast_node_module_t* parser_t::parse(const char* source) {
		auto all = this->parse_all(source);
		return all.empty() ? nullptr : all.front();
	}

	std::vector<ast_node_module_t*> parser_t::parse_all(const char* source) {
		std::vector<ast_node_module_t*> out;
		this->clear();
		this->scanner->ready(source);
		this->next_token();

		while(this->token().type != token_t::EOS) {
			ast_node_module_t* mod = this->parse_module();
			if(mod == nullptr)
				return {};
			out.push_back(mod);
		}
		return out;
	}

	void parser_t::clear() {
		this->scanner->clear();
		this->factory->clear();
		this->errormsg.clear();
	}

	void parser_t::attach_annotations(ast_node_t* node, const std::vector<ast_node_annotation_t*>& anns) {
		if(node == nullptr)
			return;
		node->annotations = anns;
		for(auto* ann : anns) {
			if(ann != nullptr)
				ann->parent = node;
		}
	}

	String parser_t::extract_export_name(ast_node_t* node) {
		if(node == nullptr)
			return "";
		switch(node->category) {
			case ast_category_t::STRUCT_DEF:
				return dynamic_cast<ast_node_struct_def_t*>(node)->name;
			case ast_category_t::ENUM_DEF:
				return dynamic_cast<ast_node_enum_def_t*>(node)->name;
			case ast_category_t::SCHEMA_DEF:
				return dynamic_cast<ast_node_schema_def_t*>(node)->name;
			case ast_category_t::META_DEF:
				return dynamic_cast<ast_node_meta_def_t*>(node)->name;
			case ast_category_t::SYMBOL_DEF:
				return dynamic_cast<ast_node_symbol_def_t*>(node)->name;
			case ast_category_t::FUNC_DEF:
				return dynamic_cast<ast_node_func_def_t*>(node)->name;
			default:
				return "";
		}
	}

	String parser_t::parse_dotted_path(bool required) {
		if(!this->check_token(token_t::ID, required, false))
			return "";

		String path = this->token().value;
		this->next_token();

		while(this->check_token(token_t::DOT, false)) {
			this->next_token();
			if(!this->check_token(token_t::ID, true, false))
				return "";
			path = path + "." + this->token().value;
			this->next_token();
		}

		return path;
	}

	bool parser_t::parse_type_params(std::vector<String>& out) {
		out.clear();
		if(!this->check_token(token_t::LT, false))
			return true;

		this->next_token();
		do {
			if(!this->check_token(token_t::ID, true, false))
				return false;
			out.push_back(this->token().value);
			this->next_token();
		} while(this->check_token(token_t::COMMA, false));

		if(!this->check_token(token_t::GT))
			return false;

		return true;
	}

	bool parser_t::parse_schema_clause(ast_node_t* p, std::vector<ast_node_type_t*>& out) {
		out.clear();
		if(!this->check_token(token_t::COLON, false))
			return true;

		this->next_token();
		do {
			auto* schema = this->parse_type(p);
			if(schema == nullptr)
				return false;
			out.push_back(schema);
		} while(this->check_token(token_t::COMMA, false));

		return true;
	}

	bool parser_t::parse_generic_type_args(ast_node_t* p, std::vector<ast_node_type_t*>& out) {
		out.clear();
		if(!this->check_token(token_t::LT, false))
			return true;

		this->next_token();
		do {
			if(!out.empty() && !this->check_token(token_t::COMMA))
				return false;

			auto* arg = this->parse_type(p);
			if(arg == nullptr)
				return false;
			out.push_back(arg);
		} while(this->check_token(token_t::COMMA, false));

		if(out.empty()) {
			this->error("type arguments is empty.");
			return false;
		}

		if(!this->check_token(token_t::GT))
			return false;

		return true;
	}

	ast_node_module_t* parser_t::parse_module() {
		if(!this->check_token(token_t::MODULE, true))
			return nullptr;

		auto* module = factory->create<ast_node_module_t>(nullptr);
		module->entry = factory->create<ast_node_func_def_t>(module);

		this->next_token();
		module->name = this->parse_dotted_path();
		if(module->name.empty())
			return nullptr;
		if(!this->check_token(token_t::LCB))
			return nullptr;

		while(this->token().type != token_t::EOS) {
			if(this->check_token(token_t::RCB, false)) {
				this->next_token();
				this->check_token(token_t::SEMICOLON, false);
				break;
			}

			if(this->check_token(token_t::MODULE, false)) {
				this->error("nested module definition is not allowed.");
				return nullptr;
			}

			auto& token = this->token();
			switch(token.type) {
				case token_t::IMPORT: {
					ast_node_import_t* _import = this->parse_import(module);
					if(_import == nullptr)
						return nullptr;
					const String key = !_import->alias.isEmpty() ? _import->alias : _import->target;
					auto iter = module->imports.find(key);
					if(iter != module->imports.end()) {
						this->error_import_exists(key);
						return nullptr;
					}
					module->imports.insert(std::make_pair(key, _import));
				} break;
				case token_t::EXPORT: {
					ast_node_t* decl = nullptr;
					ast_node_export_t* _export = this->parse_export(module, &decl);
					if(_export == nullptr)
						return nullptr;
					if(!_export->moduleReexport && decl == nullptr)
						return nullptr;
					const String key = _export->moduleReexport ? _export->modulePath : _export->name;
					auto iter = module->exports.find(key);
					if(iter != module->exports.end()) {
						this->error_export_exists(key);
						return nullptr;
					}
					module->exports.insert(std::make_pair(key, _export));
					if(decl != nullptr)
						module->entry->body.push_back(reinterpret_cast<ast_node_stmt_t*>(decl));
				} break;
				default: {
					ast_node_stmt_t* stmt = this->parse_stmt(module->entry);
					if(stmt == nullptr)
						return nullptr;
					module->entry->body.push_back(stmt);
				} break;
			}
		}

		return module;
	}

	ast_node_import_t* parser_t::parse_import(ast_node_t* p) {
		if(p->category != ast_category_t::MODULE)
			return nullptr;

		if(!this->check_token(token_t::IMPORT))
			return nullptr;

		auto node = factory->create<ast_node_import_t>(p);
		if(!this->check_token(token_t::ID, true, false))
			return nullptr;

		String first = this->token().value;
		this->next_token();

		if(this->check_token(token_t::ASSIGN, false)) {
			this->next_token();
			node->alias = first;
			node->target = this->parse_dotted_path();
		} else {
			node->target = first;
			while(this->check_token(token_t::DOT, false)) {
				this->next_token();
				if(!this->check_token(token_t::ID, true, false))
					return nullptr;
				node->target = node->target + "." + this->token().value;
				this->next_token();
			}
		}

		if(node->target.empty())
			return nullptr;

		const size_t dot = node->target.rfind('.');
		node->name = dot != String::npos ? node->target.substr(dot + 1) : node->target;

		if(!this->check_token(token_t::SEMICOLON))
			return nullptr;

		return node;
	}

	ast_node_export_t* parser_t::parse_export(ast_node_t* p, ast_node_t** out_decl) {
		if(out_decl != nullptr)
			*out_decl = nullptr;

		if(p->category != ast_category_t::MODULE)
			return nullptr;

		if(!this->check_token(token_t::EXPORT))
			return nullptr;

		auto* mod = dynamic_cast<ast_node_module_t*>(p);
		const token_t::token_type next = this->token().type;
		if(next == token_t::ID) {
			auto node = factory->create<ast_node_export_t>(p);
			node->moduleReexport = true;
			node->modulePath = this->parse_dotted_path();
			if(node->modulePath.empty())
				return nullptr;
			if(mod != nullptr && node->modulePath == mod->name) {
				this->error("cannot re-export module '%s' from itself.", node->modulePath.cstr());
				return nullptr;
			}
			node->name = node->modulePath;
			if(!this->check_token(token_t::SEMICOLON))
				return nullptr;
			return node;
		}

		ast_node_t* decl = this->parse_exportable_decl(p);
		if(decl == nullptr)
			return nullptr;

		const String name = this->extract_export_name(decl);
		if(name.empty()) {
			this->error_token_unexpected();
			return nullptr;
		}

		auto node = factory->create<ast_node_export_t>(p);
		node->name = name;

		if(out_decl != nullptr)
			*out_decl = decl;

		if(decl->category == ast_category_t::SYMBOL_DEF) {
			if(!this->check_token(token_t::SEMICOLON))
				return nullptr;
		}

		return node;
	}

	ast_node_t* parser_t::parse_exportable_decl(ast_node_t* p) {
		std::vector<ast_node_annotation_t*> anns;
		if(!this->parse_annotations(p, anns))
			return nullptr;

		ast_node_t* node = nullptr;
		switch(this->token().type) {
			case token_t::STRUCT:
				node = this->parse_stmt_struct_def(p);
			break;
			case token_t::ENUM:
				node = this->parse_stmt_enum_def(p);
			break;
			case token_t::SCHEMA:
				node = this->parse_stmt_schema_def(p);
			break;
			case token_t::META:
				node = this->parse_stmt_meta_def(p);
			break;
			case token_t::FUNC: {
				auto* func = this->parse_named_func_def(p);
				node = func;
			} break;
			case token_t::VAR:
			case token_t::VAL:
				node = this->parse_stmt_symbol_def(p);
			break;
			default:
				this->error_token_unexpected();
			return nullptr;
		}

		if(node == nullptr)
			return nullptr;

		this->attach_annotations(node, anns);
		return node;
	}

	ast_node_type_t* parser_t::parse_type(ast_node_t* p) {
		if(this->check_token(token_t::FUNC, false)) {
			this->next_token();

			auto* node = factory->create<ast_node_type_t>(p);
			node->name = "func";

			std::vector<String> ignored;
			if(!this->parse_type_params(ignored))
				return nullptr;

			if(!this->check_token(token_t::LRB))
				return nullptr;

			do {
				if(this->token().type == token_t::RRB)
					break;

				if(!this->check_token(token_t::VAR, false, false) && !this->check_token(token_t::VAL, false, false))
					return nullptr;
				this->next_token();

				if(!this->check_token(token_t::ID, true, false))
					return nullptr;
				this->next_token();

				if(!this->check_token(token_t::COLON))
					return nullptr;

				auto* argType = this->parse_type(node);
				if(argType == nullptr)
					return nullptr;
				node->args.push_back(argType);
			} while(this->check_token(token_t::COMMA, false));

			if(!this->check_token(token_t::RRB))
				return nullptr;

			if(!this->check_token(token_t::ARROW))
				return nullptr;

			auto* retType = this->parse_type(node);
			if(retType == nullptr)
				return nullptr;
			node->args.push_back(retType);

			return node;
		}

		if(!this->check_token(token_t::ID, true, false))
			return nullptr;

		String name = this->token().value;
		auto* node = factory->create<ast_node_type_t>(p);
		node->name = name;
		this->next_token();

		if(this->check_token(token_t::LT, false)) {
			this->next_token();
			while(!this->check_token(token_t::GT, false)) {
				if(!node->args.empty() && !this->check_token(token_t::COMMA))
					return nullptr;

				auto* arg = this->parse_type(node);
				if(arg == nullptr)
					return nullptr;
				node->args.push_back(arg);
			}

			if(node->args.empty()) {
				this->error("type arguments is empty.");
				return nullptr;
			}
		}

		return node;
	}

	ast_node_expr_t* parser_t::parse_expr(ast_node_t* p) {
		return this->parse_expr_trinary(p);
	}

	ast_node_expr_t* parser_t::parse_expr_trinary(ast_node_t* p) {
		ast_node_expr_t* binary = this->parse_expr_binary(p, 1);
		if(binary == nullptr)
			return nullptr;

		if(this->check_token(token_t::QUESTION, false)) {
			auto* trinary = factory->create<ast_node_expr_trinary_t>(p);
			trinary->cond = binary;
			binary->parent = trinary;

			trinary->branch_true = this->parse_expr(trinary);
			if(trinary->branch_true == nullptr)
				return nullptr;

			if(!this->check_token(token_t::COLON))
				return nullptr;

			trinary->branch_false = this->parse_expr(trinary);
			if(trinary->branch_false == nullptr)
				return nullptr;

			return trinary;
		}

		return binary;
	}

	ast_node_expr_t* parser_t::parse_expr_binary(ast_node_t* p, int priority) {
		ast_node_expr_t* left = nullptr;

		if(priority < static_cast<int>(ast_binary_oper_t::MAX_LEVEL) / 100)
			left = this->parse_expr_binary(p, priority + 1);
		else
			left = this->parse_expr_unary(p);

		if(left == nullptr)
			return nullptr;

		for(;;) {
			ast_binary_oper_t oper = this->check_binary_oper(priority, false);
			if(oper == ast_binary_oper_t::UNKNOWN)
				break;

			ast_node_expr_t* right = nullptr;
			if(priority < static_cast<int>(ast_binary_oper_t::MAX_LEVEL) / 100)
				right = this->parse_expr_binary(p, priority + 1);
			else
				right = this->parse_expr_unary(p);

			if(right == nullptr) {
				this->error_token_unexpected();
				return nullptr;
			}

			auto* binary = factory->create<ast_node_expr_binary_t>(p);
			binary->op = oper;
			binary->left = left;
			binary->right = right;
			left = binary;
		}

		return left;
	}

	ast_node_expr_t* parser_t::parse_expr_unary(ast_node_t* p) {
		ast_unary_oper_t oper = this->check_unary_oper(false, true);
		ast_node_expr_t* right = nullptr;

		token_t& token = this->token();
		switch(token.type) {
			case token_t::INT_B:
			case token_t::INT_X:
			case token_t::INT_D:
				right = this->parse_literal_int(p);
			break;
			case token_t::FLOAT:
				right = this->parse_literal_float(p);
			break;
			case token_t::STRING:
				right = this->parse_literal_string(p);
			break;
			case token_t::TRUE:
			case token_t::FALSE:
				right = this->parse_literal_bool(p);
			break;
			case token_t::FUNC:
				right = this->parse_func_def(p, true);
			break;
			case token_t::ID:
			case token_t::SELF:
			case token_t::THIS:
			case token_t::LRB:
				right = this->parse_expr_suffixed(p);
			break;
			default:
				this->error_token_unexpected();
			return nullptr;
		}

		if(oper == ast_unary_oper_t::UNKNOWN)
			return right;

		auto* unary = factory->create<ast_node_expr_unary_t>(p);
		unary->op = oper;
		unary->right = right;
		right->parent = unary;
		return unary;
	}

	ast_node_expr_t* parser_t::parse_expr_suffixed(ast_node_t* p) {
		ast_node_expr_t* primary = this->parse_expr_primary(p);
		if(primary == nullptr)
			return nullptr;

		for(;;) {
			ast_node_expr_t* suffixed = nullptr;
			token_t& token = this->token();

			switch(token.type) {
				case token_t::DOT:
					suffixed = this->parse_object_ref(p, primary);
				break;
				case token_t::LSB:
					suffixed = this->parse_index_ref(p, primary);
				break;
				case token_t::LRB:
					suffixed = this->parse_func_call(p, primary, {});
				break;
				default:
					return primary;
			}

			if(suffixed == nullptr)
				return nullptr;

			primary = suffixed;
		}
	}

	ast_node_expr_t* parser_t::parse_expr_primary(ast_node_t* p) {
		if(this->check_token(token_t::LRB, false)) {
			ast_node_expr_t* expr = this->parse_expr(p);
			if(expr == nullptr)
				return nullptr;
			if(!this->check_token(token_t::RRB))
				return nullptr;
			return expr;
		}

		if(this->check_token(token_t::SELF, false) || this->check_token(token_t::THIS, false)) {
			auto* node = factory->create<ast_node_symbol_ref_t>(p);
			node->name = this->token().value;
			this->next_token();
			return node;
		}

		if(this->check_token(token_t::ID, false)) {
			String typeName = this->token().value;
			this->next_token();

			if(this->check_token(token_t::LCB, false)) {
				auto* typeNode = factory->create<ast_node_type_t>(p);
				typeNode->name = typeName;
				return this->parse_object_def(p, typeNode);
			}

			if(this->check_token(token_t::LT, false)) {
				auto* typeNode = factory->create<ast_node_type_t>(p);
				typeNode->name = typeName;
				if(!this->parse_generic_type_args(p, typeNode->args))
					return nullptr;

				if(this->check_token(token_t::LCB, false))
					return this->parse_object_def(p, typeNode);

				if(this->check_token(token_t::LRB, false)) {
					auto* sym = factory->create<ast_node_symbol_ref_t>(p);
					sym->name = typeName;
					return this->parse_func_call(p, sym, typeNode->args);
				}

				this->error_token_unexpected();
				return nullptr;
			}

			auto* node = factory->create<ast_node_symbol_ref_t>(p);
			node->name = typeName;
			return node;
		}

		this->error_token_unexpected();
		return nullptr;
	}

	ast_node_expr_t* parser_t::parse_literal_int(ast_node_t* p) {
		token_t& token = this->token();
		switch(token.type) {
			case token_t::INT_B: {
				auto* node = factory->create<ast_node_literal_int_t>(p);
				node->value = String::binstrToValue<i32_t>(token.value);
				this->next_token();
				return node;
			}
			case token_t::INT_X: {
				auto* node = factory->create<ast_node_literal_int_t>(p);
				node->value = String::hexstrToValue<i32_t>(token.value);
				this->next_token();
				return node;
			}
			case token_t::INT_D: {
				auto* node = factory->create<ast_node_literal_int_t>(p);
				node->value = String::stringToValue<i32_t>(token.value);
				this->next_token();
				return node;
			}
			default:
				this->error_token_unexpected();
			return nullptr;
		}
	}

	ast_node_expr_t* parser_t::parse_literal_float(ast_node_t* p) {
		if(this->token().type != token_t::FLOAT) {
			this->error_token_unexpected();
			return nullptr;
		}
		auto* node = factory->create<ast_node_literal_float_t>(p);
		node->value = String::stringToValue<f32_t>(this->token().value);
		this->next_token();
		return node;
	}

	ast_node_expr_t* parser_t::parse_literal_bool(ast_node_t* p) {
		if(this->token().type != token_t::TRUE && this->token().type != token_t::FALSE) {
			this->error_token_unexpected();
			return nullptr;
		}
		auto* node = factory->create<ast_node_literal_bool_t>(p);
		node->value = String::stringToValue<bool>(this->token().value);
		this->next_token();
		return node;
	}

	ast_node_expr_t* parser_t::parse_literal_string(ast_node_t* p) {
		if(this->token().type != token_t::STRING) {
			this->error_token_unexpected();
			return nullptr;
		}
		auto* node = factory->create<ast_node_literal_string_t>(p);
		node->value = this->token().value;
		this->next_token();
		return node;
	}

	ast_node_func_def_t* parser_t::parse_named_func_def(ast_node_t* p) {
		return dynamic_cast<ast_node_func_def_t*>(this->parse_func_def(p, true));
	}

	ast_node_expr_t* parser_t::parse_func_def(ast_node_t* p, bool require_body) {
		if(!this->check_token(token_t::FUNC))
			return nullptr;

		auto* node = factory->create<ast_node_func_def_t>(p);

		if(this->check_token(token_t::ID, false)) {
			token_t& ahead = this->look_ahead_token();
			if(ahead.type == token_t::LT || ahead.type == token_t::LRB) {
				node->name = this->token().value;
				this->next_token();
			}
		}

		if(!this->parse_type_params(node->typeParams))
			return nullptr;

		if(!this->parse_func_params(node))
			return nullptr;

		if(!this->check_token(token_t::ARROW))
			return nullptr;

		node->rtype = this->parse_type(node);
		if(node->rtype == nullptr)
			return nullptr;

		if(require_body) {
			if(!this->parse_func_body(node))
				return nullptr;
		} else if(this->check_token(token_t::LCB, false)) {
			if(!this->parse_func_body(node))
				return nullptr;
		}

		return node;
	}

	bool parser_t::parse_func_params(ast_node_func_def_t* node) {
		if(!this->check_token(token_t::LRB))
			return false;

		do {
			if(this->token().type == token_t::RRB)
				break;

			bool isVar = true;
			if(this->check_token(token_t::VAR, false, false))
				isVar = true;
			else if(this->check_token(token_t::VAL, false, false))
				isVar = false;
			else
				return false;
			this->next_token();

			if(!this->check_token(token_t::ID, true, false))
				return false;
			const String name = this->token().value;
			if(node->getArg(name) != nullptr) {
				this->error_token_unexpected();
				return false;
			}
			this->next_token();

			if(!this->check_token(token_t::COLON))
				return false;

			ast_node_type_t* type = this->parse_type(node);
			if(type == nullptr)
				return false;

			auto* arg = node->addArg(name);
			if(arg == nullptr) {
				this->error_token_unexpected();
				return false;
			}
			arg->name = name;
			arg->type = type;
			arg->variable = isVar;
		} while(this->check_token(token_t::COMMA, false));

		if(!this->check_token(token_t::RRB))
			return false;

		return true;
	}

	bool parser_t::parse_func_body(ast_node_func_def_t* node) {
		if(!this->check_token(token_t::LCB))
			return false;

		while(!this->check_token(token_t::RCB, false)) {
			ast_node_stmt_t* stmt = this->parse_stmt(node);
			if(stmt == nullptr)
				return false;
			node->body.push_back(stmt);
		}

		return true;
	}

	ast_node_expr_t* parser_t::parse_func_call(ast_node_t* p, ast_node_expr_t* primary, const std::vector<ast_node_type_t*>& typeArgs) {
		auto* node = factory->create<ast_node_func_ref_t>(p);
		node->typeArgs = typeArgs;

		if(node->typeArgs.empty() && this->check_token(token_t::LT, false)) {
			if(!this->parse_generic_type_args(p, node->typeArgs))
				return nullptr;
		}

		if(!this->check_token(token_t::LRB))
			return nullptr;

		while(!this->check_token(token_t::RRB, false)) {
			if(!node->args.empty() && !this->check_token(token_t::COMMA))
				return nullptr;

			ast_node_expr_t* arg = this->parse_expr(node);
			if(arg == nullptr)
				return nullptr;
			node->args.push_back(arg);
		}

		node->func = primary;
		primary->parent = node;
		return node;
	}

	ast_node_expr_t* parser_t::parse_object_def(ast_node_t* p, ast_node_type_t* type) {
		auto* node = factory->create<ast_node_object_def_t>(p);
		node->type = type;
		type->parent = node;

		if(!this->check_token(token_t::LCB))
			return nullptr;

		do {
			if(this->token().type == token_t::RCB)
				break;
			if(!this->parse_object_field(node))
				return nullptr;
		} while(this->check_token(token_t::COMMA, false));

		if(!this->check_token(token_t::RCB))
			return nullptr;

		return node;
	}

	bool parser_t::parse_object_field(ast_node_object_def_t* node) {
		if(!this->check_token(token_t::ID, true, false))
			return false;

		const String key = this->token().value;
		this->next_token();

		if(!this->check_token(token_t::ASSIGN, false) && !this->check_token(token_t::COLON, false))
			return false;

		ast_node_expr_t* expr = this->parse_expr(node);
		if(expr == nullptr)
			return false;

		node->members[key] = expr;
		return true;
	}

	ast_node_expr_t* parser_t::parse_index_ref(ast_node_t* p, ast_node_expr_t* primary) {
		auto* node = factory->create<ast_node_array_ref_t>(p);

		if(!this->check_token(token_t::LSB))
			return nullptr;

		node->key = this->parse_expr(node);
		if(node->key == nullptr)
			return nullptr;

		if(!this->check_token(token_t::RSB))
			return nullptr;

		node->obj = primary;
		primary->parent = node;
		return node;
	}

	ast_node_expr_t* parser_t::parse_object_ref(ast_node_t* p, ast_node_expr_t* primary) {
		auto* node = factory->create<ast_node_object_ref_t>(p);

		if(!this->check_token(token_t::DOT))
			return nullptr;

		if(!this->check_token(token_t::ID, true, false))
			return nullptr;

		node->key = this->token().value;
		this->next_token();

		node->obj = primary;
		primary->parent = node;
		return node;
	}

	ast_node_annotation_t* parser_t::parse_annotation(ast_node_t* p) {
		if(!this->check_token(token_t::AT))
			return nullptr;

		if(!this->check_token(token_t::ID, true, false))
			return nullptr;

		auto* node = factory->create<ast_node_annotation_t>(p);
		node->name = this->token().value;
		this->next_token();

		if(this->check_token(token_t::LRB, false)) {
			this->next_token();
			while(!this->check_token(token_t::RRB, false)) {
				if(!node->args.empty() && !this->check_token(token_t::COMMA))
					return nullptr;

				if(!this->check_token(token_t::ID, true, false))
					return nullptr;
				const String key = this->token().value;
				this->next_token();

				if(!this->check_token(token_t::ASSIGN))
					return nullptr;

				ast_node_expr_t* value = this->parse_expr(node);
				if(value == nullptr)
					return nullptr;
				node->args[key] = value;
			}
		}

		return node;
	}

	bool parser_t::parse_annotations(ast_node_t* p, std::vector<ast_node_annotation_t*>& out) {
		out.clear();
		while(this->check_token(token_t::AT, false)) {
			auto* ann = this->parse_annotation(p);
			if(ann == nullptr)
				return false;
			out.push_back(ann);
		}
		return true;
	}

	ast_node_stmt_t* parser_t::parse_stmt(ast_node_t* p) {
		std::vector<ast_node_annotation_t*> anns;
		if(!this->parse_annotations(p, anns))
			return nullptr;

		ast_node_stmt_t* stmt = nullptr;
		bool semicolon = false;

		switch(this->token().type) {
			case token_t::MODULE:
				this->error("nested module definition is not allowed.");
				return nullptr;
			case token_t::STRUCT:
				stmt = reinterpret_cast<ast_node_stmt_t*>(this->parse_stmt_struct_def(p));
			break;
			case token_t::ENUM:
				stmt = reinterpret_cast<ast_node_stmt_t*>(this->parse_stmt_enum_def(p));
			break;
			case token_t::SCHEMA:
				stmt = reinterpret_cast<ast_node_stmt_t*>(this->parse_stmt_schema_def(p));
			break;
			case token_t::META:
				stmt = reinterpret_cast<ast_node_stmt_t*>(this->parse_stmt_meta_def(p));
			break;
			case token_t::FUNC:
				stmt = reinterpret_cast<ast_node_stmt_t*>(this->parse_named_func_def(p));
			break;
			case token_t::VAR:
			case token_t::VAL:
				stmt = this->parse_stmt_symbol_def(p);
			semicolon = true;
			break;
			case token_t::BREAK:
				stmt = this->parse_stmt_break(p);
			semicolon = true;
			break;
			case token_t::CONTINUE:
				stmt = this->parse_stmt_continue(p);
			semicolon = true;
			break;
			case token_t::RETURN:
				stmt = this->parse_stmt_return(p);
			semicolon = true;
			break;
			case token_t::IF:
				stmt = reinterpret_cast<ast_node_stmt_t*>(this->parse_stmt_if(p));
			break;
			case token_t::FOR:
				stmt = reinterpret_cast<ast_node_stmt_t*>(this->parse_stmt_for(p));
			break;
			case token_t::WHILE:
				stmt = reinterpret_cast<ast_node_stmt_t*>(this->parse_stmt_while(p));
			break;
			case token_t::SWITCH:
				stmt = reinterpret_cast<ast_node_stmt_t*>(this->parse_stmt_switch(p));
			break;
			case token_t::DO:
				stmt = this->parse_stmt_do(p);
			break;
			case token_t::LCB:
				stmt = this->parse_stmt_block(p);
			break;
			default:
				stmt = this->parse_stmt_assign_or_call(p);
			semicolon = true;
		}

		if(stmt == nullptr)
			return nullptr;

		this->attach_annotations(reinterpret_cast<ast_node_t*>(stmt), anns);

		if(semicolon && !this->check_token(token_t::SEMICOLON))
			return nullptr;

		return stmt;
	}

	ast_node_struct_def_t* parser_t::parse_stmt_struct_def(ast_node_t* p) {
		if(!this->check_token(token_t::STRUCT))
			return nullptr;

		auto* node = factory->create<ast_node_struct_def_t>(p);

		if(!this->check_token(token_t::ID, true, false))
			return nullptr;
		node->name = this->token().value;
		this->next_token();

		if(!this->parse_type_params(node->typeParams))
			return nullptr;

		if(!this->parse_schema_clause(p, node->schemas))
			return nullptr;

		if(!this->check_token(token_t::LCB))
			return nullptr;

		do {
			if(this->token().type == token_t::RCB)
				break;

			if(this->token().type == token_t::FUNC) {
				auto* method = this->parse_func_def(node, true);
				if(method == nullptr)
					return nullptr;
				node->methods.push_back(dynamic_cast<ast_node_func_def_t*>(method));
			} else if(!this->parse_stmt_struct_member(node)) {
				return nullptr;
			}
		} while(this->check_token(token_t::SEMICOLON, false));

		if(!this->check_token(token_t::RCB))
			return nullptr;

		this->check_token(token_t::SEMICOLON, false);
		return node;
	}

	bool parser_t::parse_stmt_struct_member(ast_node_struct_def_t* p) {
		bool isConst = false;
		switch(this->token().type) {
			case token_t::VAL:
				isConst = true;
				this->next_token();
			break;
			case token_t::VAR:
				isConst = false;
				this->next_token();
			break;
			default:
				return false;
		}

		if(!this->check_token(token_t::ID, true, false))
			return false;

		const String& name = this->token().value;
		auto* node = p->addMember(name);
		if(node == nullptr) {
			this->error_token_unexpected();
			return false;
		}
		this->next_token();

		if(!this->check_token(token_t::COLON))
			return false;

		node->type = this->parse_type(p);
		if(node->type == nullptr)
			return false;

		node->isConst = isConst;

		if(this->check_token(token_t::ASSIGN, false)) {
			node->value = this->parse_expr(p);
			if(node->value == nullptr)
				return false;
		}

		return true;
	}

	ast_node_enum_def_t* parser_t::parse_stmt_enum_def(ast_node_t* p) {
		if(!this->check_token(token_t::ENUM))
			return nullptr;

		auto* node = factory->create<ast_node_enum_def_t>(p);

		if(!this->check_token(token_t::ID, true, false))
			return nullptr;
		node->name = this->token().value;
		this->next_token();

		if(!this->check_token(token_t::LCB))
			return nullptr;

		int index = 0;
		do {
			if(this->token().type == token_t::RCB)
				break;

			if(!this->check_token(token_t::ID, true, false))
				return nullptr;

			const String memName = this->token().value;
			if(node->members.find(memName) != node->members.end()) {
				this->error_token_unexpected();
				return nullptr;
			}
			this->next_token();

			i32_t memValue = index;
			if(this->check_token(token_t::ASSIGN, false)) {
				auto memExpr = this->parse_literal_int(node);
				if(memExpr == nullptr)
					return nullptr;
				auto memIntExpr = dynamic_cast<ast_node_literal_int_t*>(memExpr);
				memValue = static_cast<i32_t>(memIntExpr->value);
				index = memValue;
			}

			node->members[memName] = memValue;
			index += 1;
		} while(this->check_token(token_t::COMMA, false));

		if(!this->check_token(token_t::RCB))
			return nullptr;

		this->check_token(token_t::SEMICOLON, false);
		return node;
	}

	ast_node_schema_def_t* parser_t::parse_stmt_schema_def(ast_node_t* p) {
		if(!this->check_token(token_t::SCHEMA))
			return nullptr;

		auto* node = factory->create<ast_node_schema_def_t>(p);

		if(!this->check_token(token_t::ID, true, false))
			return nullptr;
		node->name = this->token().value;
		this->next_token();

		if(!this->parse_type_params(node->typeParams))
			return nullptr;

		if(!this->parse_schema_clause(p, node->bases))
			return nullptr;

		if(!this->check_token(token_t::LCB))
			return nullptr;

		do {
			if(this->token().type == token_t::RCB)
				break;
			if(!this->parse_stmt_schema_member(node))
				return nullptr;
		} while(this->check_token(token_t::SEMICOLON, false));

		if(!this->check_token(token_t::RCB))
			return nullptr;

		this->check_token(token_t::SEMICOLON, false);
		return node;
	}

	bool parser_t::parse_stmt_schema_member(ast_node_schema_def_t* p) {
		if(this->check_token(token_t::FUNC, false)) {
			auto* method = this->parse_func_def(p, false);
			if(method == nullptr)
				return false;
			if(!this->check_token(token_t::SEMICOLON))
				return false;
			p->methods.push_back(dynamic_cast<ast_node_func_def_t*>(method));
			return true;
		}

		bool isConst = false;
		switch(this->token().type) {
			case token_t::VAL:
				isConst = true;
				this->next_token();
			break;
			case token_t::VAR:
				isConst = false;
				this->next_token();
			break;
			default:
				return false;
		}

		if(!this->check_token(token_t::ID, true, false))
			return false;

		const String& name = this->token().value;
		auto* node = p->addMember(name);
		if(node == nullptr) {
			this->error_token_unexpected();
			return false;
		}
		this->next_token();

		if(!this->check_token(token_t::COLON))
			return false;

		node->type = this->parse_type(p);
		if(node->type == nullptr)
			return false;

		node->isConst = isConst;

		if(this->check_token(token_t::ASSIGN, false)) {
			node->value = this->parse_expr(p);
			if(node->value == nullptr)
				return false;
		}

		if(!this->check_token(token_t::SEMICOLON))
			return false;

		return true;
	}

	ast_node_meta_def_t* parser_t::parse_stmt_meta_def(ast_node_t* p) {
		if(!this->check_token(token_t::META))
			return nullptr;

		auto* node = factory->create<ast_node_meta_def_t>(p);

		if(!this->check_token(token_t::ID, true, false))
			return nullptr;
		node->name = this->token().value;
		this->next_token();

		if(!this->check_token(token_t::LCB))
			return nullptr;

		do {
			if(this->token().type == token_t::RCB)
				break;
			if(!this->parse_stmt_meta_field(node))
				return nullptr;
		} while(this->check_token(token_t::SEMICOLON, false));

		if(!this->check_token(token_t::RCB))
			return nullptr;

		this->check_token(token_t::SEMICOLON, false);
		return node;
	}

	bool parser_t::parse_stmt_meta_field(ast_node_meta_def_t* p) {
		if(!this->check_token(token_t::VAL))
			return false;

		if(!this->check_token(token_t::ID, true, false))
			return false;

		const String& name = this->token().value;
		auto* node = p->addField(name);
		if(node == nullptr) {
			this->error_token_unexpected();
			return false;
		}
		this->next_token();

		if(!this->check_token(token_t::COLON))
			return false;

		node->type = this->parse_type(p);
		if(node->type == nullptr)
			return false;

		if(this->check_token(token_t::ASSIGN, false)) {
			node->value = this->parse_expr(p);
			if(node->value == nullptr)
				return false;
		}

		return true;
	}

	ast_node_symbol_def_t* parser_t::parse_stmt_symbol_def(ast_node_t* p) {
		if(!this->check_token(token_t::VAR, false, false) && !this->check_token(token_t::VAL, false, false))
			return nullptr;

		auto* node = factory->create<ast_node_symbol_def_t>(p);
		node->variable = this->token().type == token_t::VAR;
		this->next_token();

		if(!this->check_token(token_t::ID, true, false))
			return nullptr;
		node->name = this->token().value;
		this->next_token();

		if(this->check_token(token_t::COLON, false)) {
			node->type = this->parse_type(p);
			if(node->type == nullptr)
				return nullptr;
		}

		if(!this->check_token(token_t::ASSIGN, true))
			return nullptr;

		node->value = this->parse_expr(node);
		if(node->value == nullptr)
			return nullptr;

		return node;
	}

	ast_node_continue_t* parser_t::parse_stmt_continue(ast_node_t* p) {
		if(!this->check_token(token_t::CONTINUE))
			return nullptr;
		return factory->create<ast_node_continue_t>(p);
	}

	ast_node_break_t* parser_t::parse_stmt_break(ast_node_t* p) {
		if(!this->check_token(token_t::BREAK))
			return nullptr;
		return factory->create<ast_node_break_t>(p);
	}

	ast_node_return_t* parser_t::parse_stmt_return(ast_node_t* p) {
		if(!this->check_token(token_t::RETURN))
			return nullptr;

		auto* node = factory->create<ast_node_return_t>(p);
		if(this->check_token(token_t::SEMICOLON, false, false))
			return node;

		node->value = this->parse_expr(node);
		if(node->value == nullptr)
			return nullptr;

		return node;
	}

	ast_node_if_t* parser_t::parse_stmt_if(ast_node_t* p) {
		if(!this->check_token(token_t::IF))
			return nullptr;

		auto* node = factory->create<ast_node_if_t>(p);

		if(!this->check_token(token_t::LRB))
			return nullptr;

		node->cond = this->parse_expr(node);
		if(node->cond == nullptr)
			return nullptr;

		if(!this->check_token(token_t::RRB))
			return nullptr;

		node->branch_true = this->parse_stmt(node);
		if(node->branch_true == nullptr)
			return nullptr;

		if(this->check_token(token_t::ELSE, false)) {
			node->branch_false = this->parse_stmt(node);
			if(node->branch_false == nullptr)
				return nullptr;
		}

		return node;
	}

	ast_node_for_t* parser_t::parse_stmt_for(ast_node_t* p) {
		if(!this->check_token(token_t::FOR))
			return nullptr;

		auto* node = factory->create<ast_node_for_t>(p);

		if(!this->check_token(token_t::LRB))
			return nullptr;

		node->init = this->parse_stmt_symbol_def(node);
		if(node->init == nullptr)
			return nullptr;
		if(!this->check_token(token_t::SEMICOLON))
			return nullptr;

		node->cond = this->parse_expr(node);
		if(node->cond == nullptr)
			return nullptr;
		if(!this->check_token(token_t::SEMICOLON))
			return nullptr;

		switch(this->token().type) {
			case token_t::VAR:
			case token_t::VAL:
				node->step = this->parse_stmt_symbol_def(node);
			break;
			default:
				node->step = this->parse_stmt_assign_or_call(node);
			break;
		}
		if(node->step == nullptr)
			return nullptr;

		if(!this->check_token(token_t::RRB))
			return nullptr;

		node->body = this->parse_stmt(node);
		if(node->body == nullptr)
			return nullptr;

		return node;
	}

	ast_node_while_t* parser_t::parse_stmt_while(ast_node_t* p) {
		if(!this->check_token(token_t::WHILE))
			return nullptr;

		auto* node = factory->create<ast_node_while_t>(p);

		if(!this->check_token(token_t::LRB))
			return nullptr;

		node->cond = this->parse_expr(node);
		if(node->cond == nullptr)
			return nullptr;

		if(!this->check_token(token_t::RRB))
			return nullptr;

		node->body = this->parse_stmt(node);
		if(node->body == nullptr)
			return nullptr;

		return node;
	}

	ast_node_switch_t* parser_t::parse_stmt_switch(ast_node_t* p) {
		if(!this->check_token(token_t::SWITCH))
			return nullptr;

		auto* node = factory->create<ast_node_switch_t>(p);

		if(!this->check_token(token_t::LRB))
			return nullptr;

		node->expr = this->parse_expr(node);
		if(node->expr == nullptr)
			return nullptr;

		if(!this->check_token(token_t::RRB))
			return nullptr;

		if(!this->check_token(token_t::LCB))
			return nullptr;

		while(!this->check_token(token_t::RCB, false)) {
			if(this->check_token(token_t::CASE, false)) {
				this->next_token();
				ast_node_switch_t::case_t switchCase;
				switchCase.value = this->parse_expr(node);
				if(switchCase.value == nullptr)
					return nullptr;

				if(!this->check_token(token_t::COLON))
					return nullptr;

				while(this->token().type != token_t::CASE
					&& this->token().type != token_t::DEFAULT
					&& this->token().type != token_t::RCB) {
					auto* stmt = this->parse_stmt(node);
					if(stmt == nullptr)
						return nullptr;
					switchCase.body.push_back(stmt);
				}

				node->cases.push_back(switchCase);
			} else if(this->check_token(token_t::DEFAULT, false)) {
				this->next_token();
				if(!this->check_token(token_t::COLON))
					return nullptr;

				while(this->token().type != token_t::RCB) {
					auto* stmt = this->parse_stmt(node);
					if(stmt == nullptr)
						return nullptr;
					node->default_body.push_back(stmt);
				}
			} else {
				this->error_token_unexpected();
				return nullptr;
			}
		}

		return node;
	}

	ast_node_block_t* parser_t::parse_stmt_do(ast_node_t* p) {
		if(!this->check_token(token_t::DO))
			return nullptr;
		return this->parse_stmt_block(p, true);
	}

	ast_node_block_t* parser_t::parse_stmt_block(ast_node_t* p, bool breakable) {
		if(!this->check_token(token_t::LCB))
			return nullptr;

		auto* node = factory->create<ast_node_block_t>(p);
		node->breakable = breakable;

		while(!this->check_token(token_t::RCB, false)) {
			auto* stmt = this->parse_stmt(node);
			if(stmt == nullptr)
				return nullptr;
			node->stmts.push_back(stmt);
			this->check_token(token_t::SEMICOLON, false);
		}

		return node;
	}

	ast_node_stmt_t* parser_t::parse_stmt_assign_or_call(ast_node_t* p) {
		ast_node_expr_t* left = this->parse_expr_suffixed(p);
		if(left == nullptr)
			return nullptr;

		if(this->check_token(token_t::ASSIGN, false)) {
			auto* node = factory->create<ast_node_assign_t>(p);
			node->left = left;
			left->parent = node;
			node->right = this->parse_expr(node);
			if(node->right == nullptr)
				return nullptr;
			return node;
		}

		if(left->category == ast_category_t::FUNC_REF) {
			auto* node = factory->create<ast_node_invoke_t>(p);
			node->expr = dynamic_cast<ast_node_func_ref_t*>(left);
			left->parent = node;
			return node;
		}

		this->error_token_unexpected();
		return nullptr;
	}

	void parser_t::next_token() {
		scanner->next_token();
	}

	token_t& parser_t::token() {
		return scanner->token();
	}

	token_t& parser_t::look_ahead_token() {
		return scanner->look_ahead_token();
	}

	bool parser_t::check_token(const token_t::token_type& tokenType, bool required, bool movenext) {
		if(scanner->token().type != tokenType) {
			if(required)
				this->error_token_unexpected();
			return false;
		}
		if(movenext)
			scanner->next_token();
		return true;
	}

	ast_unary_oper_t parser_t::check_unary_oper(bool required, bool movenext) {
		ast_unary_oper_t oper = ast_unary_oper_t::UNKNOWN;
		switch(scanner->token().type) {
			case token_t::ADD:
				oper = ast_unary_oper_t::POS;
			break;
			case token_t::SUB:
				oper = ast_unary_oper_t::NEG;
			break;
			case token_t::NOT:
				oper = ast_unary_oper_t::NOT;
			break;
			case token_t::FLIP:
				oper = ast_unary_oper_t::FLIP;
			break;
			default:
				break;
		}
		if(oper == ast_unary_oper_t::UNKNOWN) {
			if(required)
				this->error_token_unexpected();
			return oper;
		}
		if(movenext)
			scanner->next_token();
		return oper;
	}

	ast_binary_oper_t parser_t::check_binary_oper(int priority, bool required, bool movenext) {
		ast_binary_oper_t oper = ast_binary_oper_t::UNKNOWN;
		switch(scanner->token().type) {
			case token_t::OR2:
				oper = ast_binary_oper_t::OR;
			break;
			case token_t::AND2:
				oper = ast_binary_oper_t::AND;
			break;
			case token_t::EQ:
				oper = ast_binary_oper_t::EQ;
			break;
			case token_t::GT:
				oper = ast_binary_oper_t::GT;
			break;
			case token_t::LT:
				oper = ast_binary_oper_t::LT;
			break;
			case token_t::GE:
				oper = ast_binary_oper_t::GE;
			break;
			case token_t::LE:
				oper = ast_binary_oper_t::LE;
			break;
			case token_t::NE:
				oper = ast_binary_oper_t::NE;
			break;
			case token_t::ADD:
				oper = ast_binary_oper_t::ADD;
			break;
			case token_t::SUB:
				oper = ast_binary_oper_t::SUB;
			break;
			case token_t::MUL:
				oper = ast_binary_oper_t::MUL;
			break;
			case token_t::DIV:
				oper = ast_binary_oper_t::DIV;
			break;
			case token_t::MOD:
				oper = ast_binary_oper_t::MOD;
			break;
			case token_t::AND:
				oper = ast_binary_oper_t::BIT_AND;
			break;
			case token_t::OR:
				oper = ast_binary_oper_t::BIT_OR;
			break;
			case token_t::XOR:
				oper = ast_binary_oper_t::BIT_XOR;
			break;
			case token_t::SHIFT_L:
				oper = ast_binary_oper_t::SHIFT_L;
			break;
			case token_t::SHIFT_R:
				oper = ast_binary_oper_t::SHIFT_R;
			break;
			default:
				break;
		}
		if((int) oper / 100 != priority)
			oper = ast_binary_oper_t::UNKNOWN;
		if(oper == ast_binary_oper_t::UNKNOWN) {
			if(required)
				this->error_token_unexpected();
			return oper;
		}
		if(movenext)
			scanner->next_token();
		return oper;
	}

	void parser_t::error(const char* fmt, ...) {
		String message;
		_FormatVA(message, fmt);
		errormsg = String::format("%s at %d, %d.\n", message.cstr(), scanner->line(), scanner->column());
	}

	void parser_t::error_token_unexpected() {
		token_t& token = scanner->token();
		const char* value = token.value.cstr();
		if(token.type == token_t::EOS)
			this->error("Unexpected eos");
		else
			this->error("Unexpected token '%s'", value);
	}

	void parser_t::error_import_exists(const String& entry) {
		this->error("The import entry '%s' is already existed.", entry.cstr());
	}

	void parser_t::error_export_exists(const String& entry) {
		this->error("The export entry '%s' is already existed.", entry.cstr());
	}

	const String& parser_t::error() const {
		return errormsg;
	}
}
