#include "app.h"
#include "./parser.h"
#include "../sema/sema-analyzer.h"
#include "../cpp/cpp-backend.h"

using namespace eokas;

#include <stdio.h>

static void eokas_main(const String& fileName);
static void about(void);
static void help(void);
static void bad_command(const char* command);
static String read_text_file(String& filePath);

int main(int argc, char** argv) {
    cli::Command program(argv[0]);

    program.action([&](const cli::Command& cmd) -> void {
        about();
    });

    program.subCommand("help", "")
        .action([&](const cli::Command& cmd) -> void {
            help();
        });

    program.subCommand("compile", "")
        .option("--file,-f", "", "")
        .action([&](const cli::Command& cmd) -> void {
            auto file = cmd.fetchValue("--file").string();
            if (file.isEmpty())
                throw std::invalid_argument("The argument 'file' is empty.");

            printf("=> Source file: %s\n", file.cstr());

            eokas_main(file);
        });

    program.subCommand("run", "")
        .option("--file,-f", "", "")
        .action([&](const cli::Command& cmd) -> void {
            auto file = cmd.fetchValue("--file").string();
            if (file.isEmpty())
                throw std::invalid_argument("The argument 'file' is empty.");
            if (!File::exists(file))
                throw std::invalid_argument(
                        String::format("The source file '%s' is not found.", file.cstr()).cstr());

            printf("=> Source file: %s\n", file.cstr());

            eokas_main(file);
        });

    try {
        program.exec(argc, argv);
        return 0;
    }
    catch (const std::exception& e) {
        printf("\033[31mERROR: %s\033[0m", e.what());
        return -1;
    }
}

static void eokas_main(const String& file) {
    std::map<String, ast_node_module_t*> modules;

    // Keep the parsers (and therefore the AST nodes owned by their factories)
    // alive until semantic analysis and code generation have finished.
    std::vector<std::unique_ptr<parser_t>> parsers;

    std::vector<String> filesToParse;
    filesToParse.push_back(file);

    for(size_t index = 0; index < filesToParse.size(); index++) {
        String& filePath = filesToParse[index];

        // parsed already
        if(modules.find(filePath) != modules.end()) {
            continue;
        }

        String source = read_text_file(filePath);
        printf("=> Source code:\n");
        printf("------------------------------------------\n");
        printf("%s\n", source.replace("%", "%%").cstr());
        printf("------------------------------------------\n");

        parsers.push_back(std::make_unique<parser_t>());
        parser_t& parser = *parsers.back();
        ast_node_module_t* node = parser.parse(source.cstr());
        if (node == nullptr) {
            const String& error = parser.error();
            printf("ERROR: %s\n", error.cstr());
            return;
        }

        if(!node->imports.empty()) {
            String fileHome = File::basePath(filePath);
            for(auto& item : node->imports) {
                String targetPath = item.second->target;

                // If the target is a relative path, it refers to a
                // location relative to the current file.
                if(targetPath.startsWith(".")) {
                    targetPath = File::combinePath(fileHome, targetPath);
                    targetPath = File::absolutePath(targetPath);
                }

                auto iter = std::find(filesToParse.begin(), filesToParse.end(), targetPath);
                if(iter == filesToParse.end()) {
                    filesToParse.push_back(targetPath);
                }
            }
        }

        modules.insert(std::make_pair(filePath, node));
    }

    ast_node_module_t* mainNode = modules[file];
    if (mainNode == nullptr) {
        printf("ERROR: main module is not parsed.\n");
        return;
    }

    // Semantic analysis works per-module. Analyze dependencies first (reverse
    // parse order: imported modules are appended after their importer) so that
    // imports can be resolved against already-analyzed modules.
    sema_program_t program;
    for (size_t i = filesToParse.size(); i-- > 0;) {
        ast_node_module_t* n = modules[filesToParse[i]];
        if (n == nullptr)
            continue;
        sema_analyzer_t analyzer(&program);
        analyzer.analyze(n);
    }

    String mainName = mainNode->name.isEmpty() ? String("<main>") : mainNode->name;
    sema_module_t* mainModule = program.get_module(mainName);
    if (mainModule == nullptr) {
        printf("ERROR: main module '%s' was not analyzed.\n", mainName.cstr());
        return;
    }
    if (!mainModule->ok()) {
        printf("=> Semantic errors:\n");
        printf("------------------------------------------\n");
        printf("%s", mainModule->diagnostics().dump().replace("%", "%%").cstr());
        printf("------------------------------------------\n");
        return;
    }

    cpp_backend_t backend;
    String source = backend.generate(mainModule);
    if (source.isEmpty()) {
        printf("ERROR: %s\n", backend.error().cstr());
        return;
    }

    String outPath = String::format("%s.cpp", file.cstr());
    FileStream out(outPath, "w+");
    if (!out.open()) {
        printf("ERROR: failed to open output file '%s'.\n", outPath.cstr());
        return;
    }
    out.write((void*) source.cstr(), source.length());
    out.close();

    printf("=> Generate C++ source:\n");
    printf("------------------------------------------\n");
    printf("%s", source.replace("%", "%%").cstr());
    printf("------------------------------------------\n");
    printf("=> Output: %s\n", outPath.cstr());
}

static void about(void) {
    printf("eokas %s\n", _ELANG_VERSION);
}

static void help(void) {
    printf(
        "\n-?, -help\n"
        "\tPrint command line help message.\n"

        "\nfileName [-c] [-e] [-t]\n"
        "\tComple or Execute a file, show exec-time.\n"
   );
}

static void bad_command(const char* command) {
    printf(
        "The command '%s' is undefined in eokas. "
        "You can use the command %s to get the help infomation.\n", command, "'eokas -?' or 'eokas -help'"
   );
}

static String read_text_file(String& filePath) {
    FileStream in(filePath, "rb");
    if (!in.open())
        return "";

    size_t size = in.size();
    MemoryBuffer buffer(size);
    in.read(buffer.data(), buffer.size());
    in.close();

    String content((const char*)buffer.data(), buffer.size());

    return content;
}
