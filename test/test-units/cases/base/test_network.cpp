#include "Unit.h"
#include "base/network.h"

using namespace eokas;

EOKAS_TEST_CASE(network) {
    EOKAS_EXPECT((int)NetworkError::None == 0);
    EOKAS_EXPECT((int)OperationType::Accept != (int)OperationType::Send);

    NetworkService service;
    EOKAS_EXPECT(service.init());
    service.quit();
    return 0;
}
