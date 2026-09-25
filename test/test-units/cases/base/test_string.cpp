#include "Unit.h"

using namespace eokas;

EOKAS_TEST_CASE(string) {
    EOKAS_EXPECT(String("ab") + String("c") == "abc");
    EOKAS_EXPECT(String("").isEmpty());
    EOKAS_EXPECT(String("eokas").length() == 5);
    EOKAS_EXPECT(String("eokas").startsWith("eo"));
    EOKAS_EXPECT(String("eokas").endsWith("kas"));
    EOKAS_EXPECT(String("eokas").contains("oka"));
    EOKAS_EXPECT(String("AbC").toUpper() == "ABC");
    EOKAS_EXPECT(String("AbC").toLower() == "abc");
    EOKAS_EXPECT(String("  hi  ").trim() == "hi");
    EOKAS_EXPECT(String("a-b-a").replace("a", "x") == "x-b-x");
    EOKAS_EXPECT(String::format("%d-%s", 3, "ok") == "3-ok");
    EOKAS_EXPECT(String::repeat("ab", 3) == "ababab");

    StringVector parts;
    parts.push_back("a");
    parts.push_back("b");
    parts.push_back("c");
    EOKAS_EXPECT(String::join(parts, ",") == "a,b,c");

    StringVector split = String("a,b,c").split(",");
    EOKAS_EXPECT(split.size() == 3);
    EOKAS_EXPECT(split[0] == "a");
    EOKAS_EXPECT(split[1] == "b");
    EOKAS_EXPECT(split[2] == "c");

    return 0;
}
