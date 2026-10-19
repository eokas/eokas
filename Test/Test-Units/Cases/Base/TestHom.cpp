#include "Unit.h"

using namespace eokas;

EOKAS_TEST_CASE(hom) {
    HomNode root(HomType::Object);
    root.set("name", HomNode(String("eokas")));
    root.set("version", HomNode(1.0));
    root.set("ok", HomNode(true));
    EOKAS_EXPECT(root.isObject());
    EOKAS_EXPECT(root.get("name").asString() == "eokas");
    EOKAS_EXPECT(root.get("ok").asBoolean());
    EOKAS_EXPECT(_FloatEqual((f32_t)root.get("version").asNumber(), 1.0f));

    HomNode list(HomType::Array);
    list.add(HomNode(String("a")));
    list.add(HomNode(String("b")));
    EOKAS_EXPECT(list.isArray());
    EOKAS_EXPECT(list.get(0).asString() == "a");
    EOKAS_EXPECT(list.get(1).asString() == "b");
    list.set(1, HomNode(String("c")));
    EOKAS_EXPECT(list.get(1).asString() == "c");

    int count = 0;
    list.foreach([&](const HomNode&) { count += 1; });
    EOKAS_EXPECT(count == 2);

    HomNode copied = root;
    EOKAS_EXPECT(copied.get("name").asString() == "eokas");
    EOKAS_EXPECT(HomNode().isNull());
    return 0;
}
