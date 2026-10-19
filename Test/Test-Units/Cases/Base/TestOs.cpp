#include "Unit.h"

using namespace eokas;

EOKAS_TEST_CASE(os) {
    EOKAS_EXPECT(!OS::getSystemName().isEmpty());
    EOKAS_EXPECT(OS::getCpuCount() > 0);
    EOKAS_EXPECT(!OS::getCpuArch().isEmpty());
    String user = OS::getCurrentUser();
    String version = OS::getSystemVersion();
    String device = OS::getDeviceName();
    EOKAS_EXPECT(user.cstr() != nullptr);
    EOKAS_EXPECT(version.cstr() != nullptr);
    EOKAS_EXPECT(device.cstr() != nullptr);

    MemoryState memory;
    OS::getMemoryState(memory);
    EOKAS_EXPECT(memory.total > 0);
    EOKAS_EXPECT(memory.process > 0);

    const String key = "EOKAS_UNIT_OS";
    EOKAS_EXPECT(OS::setEnv(key, "eokas"));
    EOKAS_EXPECT(OS::getEnv(key) == "eokas");
    return 0;
}
