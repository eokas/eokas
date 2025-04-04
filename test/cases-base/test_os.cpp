
#include "../engine/main.h"
using namespace eokas;

_eokas_test_case(os)
{
    printf("OS::getSystemName: %s\n", OS::getSystemName().cstr());
    printf("OS::getSystemVersion: %s\n", OS::getSystemVersion().cstr());
    printf("OS::getDeviceName: %s\n", OS::getDeviceName().cstr());
    printf("OS::getDeviceModel: %s\n", OS::getDeviceModel().cstr());
    printf("OS::getCpuCount: %d\n", OS::getCpuCount());
    printf("OS::getEnv: JAVA_HOME=%s\n", OS::getEnv("JAVA_HOME").cstr());
    printf("OS::getCurrentUser: %s\n", OS::getCurrentUser().cstr());
    printf("OS::getMacAddress: %s\n", OS::getMacAddress().cstr());
    
    return 0;
}
