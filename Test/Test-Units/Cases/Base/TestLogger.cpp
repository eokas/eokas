#include "Unit.h"

#include <cstdio>

using namespace eokas;

namespace {

int gLogHits = 0;

SignalResult onLog(Logger::LogSignalMessage& message) {
    if (message.level == LogLevel::Notice && message.message == "hello") {
        gLogHits += 1;
    }
    return SignalResult::Continue;
}

}

EOKAS_TEST_CASE(logger) {
    const String path = "eokas-unit-logger.html";
    std::remove(path.cstr());

    Logger* logger = Logger::log(path);
    EOKAS_EXPECT(logger != nullptr);
    logger->callback.attachHandler(&onLog);
    gLogHits = 0;
    logger->message(LogLevel::Notice, "hello");
    EOKAS_EXPECT(gLogHits == 1);
    EOKAS_EXPECT(File::exists(path));
    logger->callback.detachHandler(&onLog);
    logger->close();
    std::remove(path.cstr());

    Logger::push("eokas-unit-logger-stack.html");
    Logger* stacked = Logger::log();
    EOKAS_EXPECT(stacked != nullptr);
    Logger::pop();
    stacked->close();
    std::remove("eokas-unit-logger-stack.html");
    return 0;
}
