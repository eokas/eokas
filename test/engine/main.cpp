
#include "./main.h"
using namespace eokas;

int main(int argc, char **argv)
{
    if(argc == 1)
    {
        return test_case_set_t::instance().exec();
    }

    if(argc == 2)
    {
        String caseName(argv[1]);
        return test_case_set_t::instance().exec(caseName);
    }
	
    return 0;
}
