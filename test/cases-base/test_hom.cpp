
#include "../engine/main.h"
using namespace eokas;

_eokas_test_case(hom)
{
    HomNode node(HomType::Object);
    node.set("name", "eokas-hom");
    node.set("version", "1.0.0");

    printf("obj.name: %s\n", node.get("name").asString().cstr());
    printf("obj.version: %s\n", node.get("version").asString().cstr());

    return 0;
}