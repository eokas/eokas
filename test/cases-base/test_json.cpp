
#include "../engine/main.h"
using namespace eokas;

_eokas_test_case(json)
{
    String str = "{ "
        "\"name\"=\"eokas-json\", "
        "\"version\"=\"0.0.1\", "
        "\"files\"= [ "
            "\"README.md\","
            "\"package.json\""
        "],"
        "\"index\": 100"
    "}";

    /*
    auto obj = static_cast<HomObject*>(JSON::parse(str).get());
    printf("{\n");
    printf("\tname: %s\n", obj->getString("name").cstr());
    printf("\tversion: %s\n", obj->getString("version").cstr());
    printf("\tindex: %f\n", obj->getNumber("index"));
    printf("\tfiles: [\n");
    auto files = obj->getArray("files");
    for(auto& f : files) {
        auto p = static_cast<HomString*>(f.get());
        printf("\t\t%s,\n", p->value.cstr());
    }
    printf("\t]\n");
    printf("}\n");

    auto obj = JSON.parse(str);
    obj->getString("");
    obj->getString("");
    obj->getNumber("");
    obj->getBoolean("");
    auto files = obj->getArray("");
    for(auto& f : files) {
        f.get();
    }
    */

    return 0;
}
