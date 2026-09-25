#include "Unit.h"

int main(int argc, char** argv) {
    if (argc <= 1) {
        return eokas::unit::Suite::instance().execAll();
    }

    int failed = 0;
    for (int i = 1; i < argc; ++i) {
        if (eokas::unit::Suite::instance().execOne(argv[i]) != 0) {
            failed += 1;
        }
    }
    return failed == 0 ? 0 : 1;
}
