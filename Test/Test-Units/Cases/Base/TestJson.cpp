#include "Unit.h"

using namespace eokas;

EOKAS_TEST_CASE(json) {
    EOKAS_EXPECT(JSON::stringify(HomNode()) == "null");
    EOKAS_EXPECT(JSON::stringify(HomNode(true)) == "true");
    EOKAS_EXPECT(JSON::stringify(HomNode(String("eokas"))) == "\"eokas\"");

    HomNode parsed = JSON::parse("{\"name\":\"eokas\",\"ok\":true,\"files\":[\"README.md\",\"package.json\"]}");
    EOKAS_EXPECT(parsed.isObject());
    EOKAS_EXPECT(parsed.get("name").asString() == "eokas");
    EOKAS_EXPECT(parsed.get("ok").asBoolean());
    EOKAS_EXPECT(parsed.get("files").isArray());
    EOKAS_EXPECT(parsed.get("files").get(0).asString() == "README.md");
    EOKAS_EXPECT(parsed.get("files").get(1).asString() == "package.json");

    HomNode again = JSON::parse(JSON::stringify(HomNode(String("round"))));
    EOKAS_EXPECT(again.asString() == "round");
    return 0;
}
