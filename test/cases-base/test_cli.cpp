
#include "../engine/main.h"
using namespace eokas;

_eokas_test_case(cli)
{
    std::vector<const char*> args = {"docker", "run", "--name=eokas", "-p", "80:80", "-v", "/data:/data", "eokas/eokas" };

    cli::Command program(args[0]);
    program.action([&](const cli::Command& cmd)->void {
        printf("%s", cmd.toString().cstr());
    });

    program.subCommand("run", "")
        .option("--name,-n", "", "")
        .option("--publish,-p", "", "")
        .option("--volume,-v", "", "")
        .action([&](const cli::Command& cmd)->void{
            printf("%s", cmd.toString().cstr());
        });

    program.exec(args);

    return 0;
}
