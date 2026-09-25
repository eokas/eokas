#include "Unit.h"

using namespace eokas;

EOKAS_TEST_CASE(cli) {
    int ran = 0;
    String name;
    cli::Command program("tool", "demo");
    program.subCommand("run", "run a container")
        .option("--name,-n", "container name", "")
        .action([&](const cli::Command& cmd) {
            ran += 1;
            name = cmd.fetchValue("-n").string();
        });

    const char* argv[] = {"tool", "run", "-n", "eokas"};
    program.exec(4, argv);
    EOKAS_EXPECT(ran == 1);
    EOKAS_EXPECT(name == "eokas");
    EOKAS_EXPECT(program.fetchCommand("run").has_value());
    EOKAS_EXPECT(!program.fetchCommand("missing").has_value());
    EOKAS_EXPECT(program.toString().contains("tool"));
    return 0;
}
