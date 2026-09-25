#include "Unit.h"

#include <cstdio>

using namespace eokas;

EOKAS_TEST_CASE(io) {
    EOKAS_EXPECT(File::combinePath("a", "b") == "a/b");
    EOKAS_EXPECT(File::combinePath("a/", "b") == "a/b");
    EOKAS_EXPECT(File::basePath("dir/sub/file.txt") == "dir/sub");
    EOKAS_EXPECT(File::fileExtension("dir/sub/file.txt") == ".txt");
    EOKAS_EXPECT(File::fileNameWithoutExtension("file.txt") == "file");

    String path = "eokas-unit-io.txt";
    String content = "hello-eokas";
    EOKAS_EXPECT(File::writeText(path, content));
    EOKAS_EXPECT(File::exists(path));
    EOKAS_EXPECT(File::isFile(path));

    String loaded;
    EOKAS_EXPECT(File::readText(path, loaded));
    EOKAS_EXPECT(loaded == content);
    std::remove(path.cstr());
    EOKAS_EXPECT(!File::exists(path));

    EOKAS_EXPECT(Process::getPID() != 0);
    EOKAS_EXPECT(!Process::executingPath().isEmpty());
    EOKAS_EXPECT(!Process::workingPath().isEmpty());
    return 0;
}
