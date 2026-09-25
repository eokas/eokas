#include "Unit.h"
#include "elang/src/app/parser.h"
#include "elang/src/app/scanner.h"
#include "elang/src/cpp/cpp-backend.h"
#include "elang/src/sema/sema-analyzer.h"

using namespace eokas;

EOKAS_TEST_CASE(elang) {
    scanner_t scanner;
    scanner.ready("module demo { }");
    scanner.next_token();
    EOKAS_EXPECT(scanner.token().type == token_t::MODULE);
    scanner.next_token();
    EOKAS_EXPECT(scanner.token().type == token_t::ID);
    EOKAS_EXPECT(scanner.token().value == "demo");
    scanner.next_token();
    EOKAS_EXPECT(scanner.token().type == token_t::LCB);

    parser_t parser;
    ast_node_module_t* ast = parser.parse("module demo { }");
    EOKAS_EXPECT(ast != nullptr);
    EOKAS_EXPECT(ast->name == "demo");
    EOKAS_EXPECT(parser.error().isEmpty());

    sema_program_t program;
    sema_analyzer_t analyzer(&program);
    sema_module_t* module = analyzer.analyze(ast);
    EOKAS_EXPECT(module != nullptr);
    EOKAS_EXPECT(module->get_name() == "demo");

    cpp_backend_t backend;
    String source = backend.generate(module);
    EOKAS_EXPECT(backend.error().isEmpty());
    EOKAS_EXPECT(source.contains("demo"));
    return 0;
}
