
#include "../engine/main.h"

_eokas_test_case(hash)
{
    // MD5
    {
        auto md5 = eokas::md5("eokas-test-hash");
        printf("MD5: %s \n", md5.cstr());
        printf("MD5: %s \n", eokas::MD5().compute("eokas-test-hash").cstr());
        _eokas_test_check(md5 == "165f496c17b648d9ca4e1c8face803c5");
    }

    // SHA256
    {
        auto sha256 = eokas::sha256("eokas-test-hash");
        printf("SHA256: %s \n", sha256.cstr());
        printf("SHA256: %s \n", eokas::SHA256().compute("eokas-test-hash").cstr());
        _eokas_test_check(sha256 == "216fd0525ecaffc3b4a48fc5e98e1e69f387f2627c789df2e8b9c5e90df9c09b");
    }

    return 0;
}
