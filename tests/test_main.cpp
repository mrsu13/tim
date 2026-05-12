#include "tim_test.h"

#include <iostream>


int main()
{
    using namespace tim::test;

    for (const case_t &tc: all())
    {
        std::cout << "[TEST] " << tc.name << " ... " << std::flush;
        current_case_failures() = 0;
        tc.fn();
        if (current_case_failures() == 0)
            std::cout << "OK\n";
        else
            std::cout << "\n  " << current_case_failures() << " failure(s)\n";
    }

    std::cout << "----\n";
    if (total_failures() == 0)
        std::cout << "All tests passed (" << all().size() << " case(s)).\n";
    else
        std::cout << total_failures() << " failure(s) across "
                  << all().size() << " case(s).\n";

    return total_failures() == 0 ? 0 : 1;
}
