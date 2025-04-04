
#ifndef _EOKAS_BASE_TEST_H_
#define _EOKAS_BASE_TEST_H_

#include "../../src/base/main.h"
#include "../../src/native/main.h"

namespace eokas {
    using test_case_func_t = std::function<int()>;

    class test_case_set_t {
    public:
        static test_case_set_t &instance() {
            static test_case_set_t sInstance("Default");
            return sInstance;
        }

        test_case_set_t(const String &name)
                : name(name), cases() {}

        void add(const String &name, test_case_func_t func) {
            this->cases.insert(std::make_pair(name, func));
        }

        int exec() {
            printf("eokas exec test cases...\n");
            for (auto &item: this->cases) {
                printf("exec case: %s\n", item.first.cstr());
                int ret = item.second();
                if(ret == 0) {
                    printf("exec case: %s [succ]\n", item.first.cstr());
                }
                else {
                    printf("exec case: %s #%d [failed]\n", item.first.cstr(), ret);
                }
            }
            return 0;
        }

        int exec(const String &caseName) {
            auto iter = this->cases.find(caseName);
            if (iter == this->cases.end())
                return -1;
            return iter->second();
        }

    private:
        String name = "Default";
        std::map<String, test_case_func_t> cases = {};
    };

    struct test_add_t {
        test_add_t(const String &name, const test_case_func_t &func) {
            test_case_set_t::instance().add(name, func);
        }
    };

#define _eokas_test_case(name) \
    extern int test_##name();  \
    const eokas::test_add_t add_test_##name("test_"#name, test_##name); \
    static int test_check_index = 0; \
    int test_##name()

#define _eokas_test_check(expr) \
    test_check_index += 1; \
    if(!(expr)) { \
        printf("eokas test check failed at: %s #%d\n", __FUNCTION__, test_check_index); \
        return test_check_index; \
    }
}

#endif //_EOKAS_BASE_TEST_H_
