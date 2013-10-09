#include "gtest/gtest.h"
#include <mongo/util/net/hostandport.h>

using namespace Robomongo;

TEST(SSHInfoString, UserPasswordHostPort) 
{
    SSHInfo inf("sasha[:lalala]@localhost[:22]");
    SSHInfo inf2("localhost","sasha",22,"lalala","");
    ASSERT_EQ(inf, inf2);

    SSHInfo inf3("sasha[:]@127.122.33.11[:22]");
    SSHInfo inf4("127.122.33.11","sasha",22,"","");
    ASSERT_EQ(inf3, inf4);

    SSHInfo inf5("192.168.38.137","sasha",27017,"1023224","");
    SSHInfo inf6("sasha[:1023224]@192.168.38.137[:27017]");
    ASSERT_EQ(inf5, inf6);
}

TEST(SSLInfoString, SupportKey) 
{
    SSLInfo inf("1:11");
    SSLInfo inf2(true,"11");
    ASSERT_EQ(inf, inf2);

    SSLInfo inf3("0:");
    SSLInfo inf4(false,"");
    ASSERT_EQ(inf3, inf4);

    SSLInfo inf5(":");
    SSLInfo inf6(false,"");
    ASSERT_EQ(inf5, inf6);

    SSLInfo inf7("1:");
    SSLInfo inf8(true,"");
    ASSERT_EQ(inf7, inf8);
}

TEST(HostAndPortString, ConnectionString) 
{
    mongo::HostAndPort inf("localhost:11");
    mongo::HostAndPort inf2("localhost",11);
    ASSERT_EQ(inf, inf2);

    SSLInfo ssl("1:12");
    mongo::HostAndPort inf3("localhost:11[1:12]");    
    mongo::HostAndPort inf4("localhost",11,ssl,SSHInfo());
    ASSERT_EQ(inf3, inf4);

    SSHInfo ssh("sasha[:lalala]@localhost[:22]");
    mongo::HostAndPort inf5("localhost:11[1:12][sasha[:lalala]@localhost[:22]]");
    mongo::HostAndPort inf6("localhost",11,ssl,ssh);
    ASSERT_EQ(inf5, inf6);

    mongo::HostAndPort inf7("localhost:11[:][[:]@[:]]");
    mongo::HostAndPort inf8("localhost",11,SSLInfo(),SSHInfo());
    ASSERT_EQ(inf5, inf6);
        
    mongo::HostAndPort inf9("192.168.38.137:22[0:][sasha[:1023224]@192.168.38.137[:27017]]");
    mongo::HostAndPort inf10("192.168.38.137",22,SSLInfo(),SSHInfo("192.168.38.137","sasha",27017,"1023224",""));
    ASSERT_EQ(inf9, inf10);
}