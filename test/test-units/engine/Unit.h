#pragma once

#include "base/main.h"

#include <cstdio>

namespace eokas {
namespace unit {

using CaseFunc = std::function<int()>;

class Suite {
public:
    static Suite& instance() {
        static Suite suite;
        return suite;
    }

    void add(const String& name, CaseFunc func) {
        cases.insert(std::make_pair(name, func));
    }

    int execAll() {
        int failed = 0;
        printf("eokas unit tests: %zu cases\n", cases.size());
        for (auto& item : cases) {
            printf("case: %s\n", item.first.cstr());
            int ret = item.second();
            if (ret == 0) {
                printf("case: %s [ok]\n", item.first.cstr());
            }
            else {
                printf("case: %s [failed] %d\n", item.first.cstr(), ret);
                failed += 1;
            }
        }
        printf("eokas unit tests: %d failed\n", failed);
        return failed == 0 ? 0 : 1;
    }

    int execOne(const String& name) {
        auto iter = cases.find(name);
        if (iter == cases.end()) {
            printf("unknown unit case: %s\n", name.cstr());
            return 2;
        }
        printf("case: %s\n", name.cstr());
        int ret = iter->second();
        if (ret == 0) {
            printf("case: %s [ok]\n", name.cstr());
        }
        else {
            printf("case: %s [failed] %d\n", name.cstr(), ret);
        }
        return ret == 0 ? 0 : 1;
    }

private:
    Suite() = default;

    std::map<String, CaseFunc> cases;
};

struct Registrar {
    Registrar(const char* name, CaseFunc func) {
        Suite::instance().add(name, func);
    }
};

}
}

#define EOKAS_TEST_CASE(name) \
    static int eokas_test_case_##name(); \
    static const eokas::unit::Registrar eokas_test_registrar_##name("test_" #name, eokas_test_case_##name); \
    static int eokas_test_case_##name()

#define EOKAS_EXPECT(expr) \
    do { \
        if (!(expr)) { \
            printf("unit check failed: %s:%d: %s\n", __FILE__, __LINE__, #expr); \
            return 1; \
        } \
    } while (0)
