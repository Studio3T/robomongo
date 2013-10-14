#include "gtest/gtest.h"
#include <mongo/util/net/hostandport.h>

using namespace Robomongo;

TEST(SSHInfoString, UserPasswordHostPort) 
{
    SSHInfo inf("sasha(pass)[lalala]@localhost[:22]");
    SSHInfo inf2("localhost","sasha",22,"lalala",PublicKey());
    ASSERT_EQ(inf, inf2);

    SSHInfo inf3("sasha(pass)[]@127.122.33.11[:22]");
    SSHInfo inf4("127.122.33.11","sasha",22,"",PublicKey());
    ASSERT_EQ(inf3, inf4);

    SSHInfo inf5("192.168.38.137","sasha",27017,"1023224",PublicKey());
    SSHInfo inf6("sasha(pass)[1023224]@192.168.38.137[:27017]");
    ASSERT_EQ(inf5, inf6);
}

TEST(SSHInfoString, UserPublicPrivateKey) 
{
    SSHInfo inf("sasha(publ)[C:\\1.txt,C:\\2.txt]@localhost[:22]");
    SSHInfo inf2("localhost","sasha",22,"",PublicKey("C:\\1.txt","C:\\2.txt"));
    ASSERT_EQ(inf, inf2);

    SSHInfo inf3("sasha(publ)[/usr/bin/1.txt,/1.txt]@localhost[:22]");
    SSHInfo inf4("localhost","sasha",22,"",PublicKey("/usr/bin/1.txt","/1.txt"));
    ASSERT_EQ(inf3, inf4);

    SSHInfo inf5("sasha(publ)[D://1.txt,D://fdsfs f//3.txt]@localhost[:22]");
    SSHInfo inf6("localhost","sasha",22,"",PublicKey("D://1.txt","D://fdsfs f//3.txt"));
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

    SSHInfo ssh("sasha(pass)[lalala]@localhost[:22]");
    mongo::HostAndPort inf5("localhost:11[1:12][sasha(pass)[lalala]@localhost[:22]]");
    mongo::HostAndPort inf6("localhost",11,ssl,ssh);
    ASSERT_EQ(inf5, inf6);

    mongo::HostAndPort inf7("localhost:11[:][(pass)[]@[:]]");
    mongo::HostAndPort inf8("localhost",11,SSLInfo(),SSHInfo());
    ASSERT_EQ(inf5, inf6);
        
    mongo::HostAndPort inf9("192.168.38.137:22[0:][sasha(pass)[1023224]@192.168.38.137[:27017]]");
    mongo::HostAndPort inf10("192.168.38.137",22,SSLInfo(),SSHInfo("192.168.38.137","sasha",27017,"1023224",PublicKey()));
    ASSERT_EQ(inf9, inf10);
}