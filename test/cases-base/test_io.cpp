
#include "../engine/main.h"
using namespace eokas;

_eokas_test_case(io)
{
    printf("== File::listFolderNames\n");
    {
        auto nameList = File::listFolderNames("./");
        for (auto& name: nameList) {
            printf("folder: %s\n", name.cstr());
        }
    }
    
    printf("== File::listFileNames\n");
    {
        auto nameList = File::listFileNames("./");
        for (auto& name: nameList) {
            printf("file: %s\n", name.cstr());
        }
    }
    
    printf("== File::glob\n");
    {
        auto fileList = File::glob("./", "*.lib");
        for (auto& f: fileList) {
            printf("glob: %s\n", f.name.cstr());
        }
    }
    
    printf("== Process Info");
    {
        printf("Process ID: %d\n", Process::getPID());
        printf("Process executing path: %s\n", Process::executingPath().cstr());
        printf("Process working path: %s\n", Process::workingPath().cstr());
        
        printf("command: cmake --version \n");
        FileStream cmake = Process::open("cmake --version", "r");
        MemoryBuffer buffer(1024);
        cmake.read(buffer.data(), buffer.size());
        printf("%s\n", (char*)buffer.data());
    }
    
    return 0;
}
