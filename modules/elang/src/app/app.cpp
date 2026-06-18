#include "app.h"
#include "./parser.h"
#include "../sema/sema-analyzer.h"
#include "../cpp/cpp-backend.h"

using namespace eokas;

#include <stdio.h>
#include <functional>
#include <set>

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
    std::vector<std::unique_ptr<parser_t>> parsers;
    std::map<String, std::vector<ast_node_module_t*>> moduleFragments;
    std::map<String, std::vector<ast_node_module_t*>> fileModules;

    std::vector<String> filesToParse;
    filesToParse.push_back(file);
    std::set<String> parsedFiles;

    for (size_t index = 0; index < filesToParse.size(); index++) {
        String filePath = filesToParse[index];
        if (parsedFiles.find(filePath) != parsedFiles.end())
            continue;
        parsedFiles.insert(filePath);

        String source = read_text_file(filePath);
        printf("=> Source code:\n");
        printf("------------------------------------------\n");
        printf("%s\n", source.replace("%", "%%").cstr());
        printf("------------------------------------------\n");

        parsers.push_back(std::make_unique<parser_t>());
        parser_t& parser = *parsers.back();
        std::vector<ast_node_module_t*> mods = parser.parse_all(source.cstr());
        if (mods.empty()) {
            const String& error = parser.error();
            printf("ERROR: %s\n", error.cstr());
            return;
        }

        fileModules[filePath] = mods;

        for (ast_node_module_t* node : mods) {
            moduleFragments[node->name].push_back(node);

            if (!node->imports.empty()) {
                String fileHome = File::basePath(filePath);
                for (auto& item : node->imports) {
                    String targetPath = item.second->target;

                    if (targetPath.startsWith(".")) {
                        targetPath = File::combinePath(fileHome, targetPath);
                        targetPath = File::absolutePath(targetPath);

                        auto iter = std::find(filesToParse.begin(), filesToParse.end(), targetPath);
                        if (iter == filesToParse.end())
                            filesToParse.push_back(targetPath);
                    }
                }
            }
        }
    }

    auto entryIter = fileModules.find(file);
    if (entryIter == fileModules.end() || entryIter->second.empty()) {
        printf("ERROR: main module is not parsed.\n");
        return;
    }

    ast_node_module_t* mainNode = entryIter->second.front();

    std::vector<String> moduleOrder;
    std::set<String> scheduled;

    std::function<void(const String&)> scheduleModule = [&](const String& path) {
        if (scheduled.find(path) != scheduled.end())
            return;

        auto fragIt = moduleFragments.find(path);
        if (fragIt == moduleFragments.end())
            return;

        for (ast_node_module_t* m : fragIt->second) {
            for (auto& imp : m->imports)
                scheduleModule(imp.second->target);
        }

        moduleOrder.push_back(path);
        scheduled.insert(path);
    };

    for (auto& kv : moduleFragments)
        scheduleModule(kv.first);

    sema_program_t program;
    for (const String& modPath : moduleOrder) {
        std::vector<ast_node_module_t*>& frags = moduleFragments[modPath];
        for (size_t i = 0; i < frags.size(); i++) {
            sema_analyzer_t analyzer(&program);
            analyzer.analyze(frags[i], i > 0);
        }
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
