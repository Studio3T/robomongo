#undef NDEBUG

#include <sstream>
#include <iostream>
#include <assert.h>
#include <limits>

void dassert(const std::string&, double);

int main(int argc, char *argv[], char** envp)
{
    dassert("9.8", 9.8);
    dassert("9.9", 9.9);
    dassert("9.98", 9.98);
    dassert("9.987", 9.987);
    dassert("9.9876", 9.9876);
    dassert("9.98765", 9.98765);
    dassert("9.987654", 9.987654);
    dassert("9.9876543", 9.9876543);
    dassert("9.98765432", 9.98765432);
    dassert("9.987654321", 9.987654321);
    dassert("9.987654321", 9.9876543210);
    dassert("9.98765432109", 9.98765432109);
    dassert("9.987654321098", 9.987654321098);
    dassert("9.9876543210987", 9.9876543210987);
    dassert("9.98765432109876", 9.98765432109876);
    dassert("9.98765432109876", 9.987654321098765);
    dassert("9.98765432109877", 9.9876543210987654);
    dassert("3.1415", 3.1415);
    dassert("1.1", 1.1);
    dassert("9.7", 9.7);

    return 0;
}

void dassert(const std::string &text, double d) {
    std::stringstream s;
    s.precision(std::numeric_limits<double>::digits10);
    s << d;
    std::cout << "Checking " << text << " - ";
    assert(text == s.str());
    std::cout << "Correct. " << std::endl;
}
