#undef NDEBUG

#include <sstream>
#include <iostream>
#include <assert.h>
#include <limits>
#include <mongo/util/exit_code.h>
#include <mongo/util/net/hostandport.h>

namespace mongo {
    extern bool isShell;
    void logProcessDetailsForLogRotate() {}
    void exitCleanly(ExitCode code) {}
}

void testHostAndPort() {
    mongo::HostAndPort hp = mongo::HostAndPort("127.0.0.1", 20017);
    assert(hp.toString() == "127.0.0.1:20017");

    hp = mongo::HostAndPort("2a03:b0c0:3:d0::f3:1001", 20017);
    assert(hp.toString() == "[2a03:b0c0:3:d0::f3:1001]:20017");

    // toString() will wrap IPv6 address in brackets even if they
    // are already exist. This is true at least for MongoDB 3.2
    hp = mongo::HostAndPort("[2a03:b0c0:3:d0::f3:1001]", 20017);
    assert(hp.toString() == "[[2a03:b0c0:3:d0::f3:1001]]:20017");

    hp = mongo::HostAndPort("[2a03:b0c0:3:d0::f3:1001", 20017);
    assert(hp.toString() == "[[2a03:b0c0:3:d0::f3:1001]:20017");
}

void precisionAssert(const std::string &text, double d) {
    std::stringstream s;
    s.precision(std::numeric_limits<double>::digits10);
    s << d;
    if (d == (long long)d)
        s << ".0";
    std::cout << "Checking " << text << " - ";
    assert(text == s.str());
    std::cout << "Correct. " << std::endl;
}

void testPrecision() {
    precisionAssert("-9.987654321", -9.987654321);
    precisionAssert("-1.0", -1.0);
    precisionAssert("-0.0", -0.0);
    precisionAssert("0.0", 0.0);
    precisionAssert("9.0", 9.0);
    precisionAssert("9.8", 9.8);
    precisionAssert("9.9", 9.9);
    precisionAssert("9.98", 9.98);
    precisionAssert("9.987", 9.987);
    precisionAssert("9.9876", 9.9876);
    precisionAssert("9.98765", 9.98765);
    precisionAssert("9.987654", 9.987654);
    precisionAssert("9.9876543", 9.9876543);
    precisionAssert("9.98765432", 9.98765432);
    precisionAssert("9.987654321", 9.987654321);
    precisionAssert("9.987654321", 9.9876543210);
    precisionAssert("9.98765432109", 9.98765432109);
    precisionAssert("9.987654321098", 9.987654321098);
    precisionAssert("9.9876543210987", 9.9876543210987);
    precisionAssert("9.98765432109876", 9.98765432109876);
    precisionAssert("9.98765432109876", 9.987654321098765);
    precisionAssert("9.98765432109877", 9.9876543210987654);
    precisionAssert("3.1415", 3.1415);
    precisionAssert("1.1", 1.1);
    precisionAssert("9.7", 9.7);
}

int main(int argc, char *argv[], char** envp)
{
    testHostAndPort();
    testPrecision();
    return 0;
}
