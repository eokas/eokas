#include "Unit.h"

using namespace eokas;

namespace {

int gSignalHits = 0;

SignalResult onSignal(int value) {
    gSignalHits += value;
    return SignalResult::Continue;
}

SignalResult onBreak(int) {
    return SignalResult::Break;
}

}

EOKAS_TEST_CASE(signal) {
    Signal<int> signal;
    EOKAS_EXPECT(!signal.hasHandler());
    signal.attachHandler(&onSignal);
    signal.attachHandler(&onBreak);
    signal.attachHandler(&onSignal);
    EOKAS_EXPECT(signal.hasHandler());

    gSignalHits = 0;
    signal(4);
    EOKAS_EXPECT(gSignalHits == 4);

    signal.detachHandler(&onBreak);
    gSignalHits = 0;
    signal(2);
    EOKAS_EXPECT(gSignalHits == 4);
    signal.clearHandlers();
    EOKAS_EXPECT(!signal.hasHandler());
    return 0;
}
