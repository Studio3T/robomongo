#include "gtest/gtest.h"
#include <mongo/util/net/hostandport.h>

using namespace Robomongo;

TEST(SSHInfoString, UserPasswordHostPort) 
{    
    
    SSHInfo inf2("localhost",22,"sasha","lalala",PublicKey(),SSHInfo::PASSWORD);
    SSHInfo inf(inf2.toBSONObj().getField("SSH"));
    ASSERT_EQ(inf, inf2);
    
    SSHInfo inf4("127.122.33.11",22,"sasha","",PublicKey(),SSHInfo::UNKNOWN);
    SSHInfo inf3(inf4.toBSONObj().getField("SSH"));
    ASSERT_EQ(inf3, inf4);

    SSHInfo inf5("192.168.38.137",27017,"sasha","1023224",PublicKey(),SSHInfo::PASSWORD);
    SSHInfo inf6(inf5.toBSONObj().getField("SSH"));
    ASSERT_EQ(inf5, inf6);
}

TEST(SSHInfoString, UserPublicPrivateKey) 
{    
    SSHInfo inf2("localhost",22,"sasha","",PublicKey("C:\\1.txt","C:\\2.txt"),SSHInfo::PUBLICKEY);
    SSHInfo inf(inf2.toBSONObj().getField("SSH"));
    ASSERT_EQ(inf, inf2);
    
    SSHInfo inf4("localhost",22,"sasha","",PublicKey("/usr/bin/1.txt","/1.txt"),SSHInfo::PUBLICKEY);
    SSHInfo inf3(inf4.toBSONObj().getField("SSH"));
    ASSERT_EQ(inf3, inf4);
    
    SSHInfo inf6("localhost",22,"sasha","",PublicKey("D://1.txt","D://fdsfs f//3.txt","123"),SSHInfo::PUBLICKEY);
    SSHInfo inf5(inf6.toBSONObj().getField("SSH"));
    ASSERT_EQ(inf5, inf6);
}

TEST(SSLInfoString, SupportKey) 
{
    
    SSLInfo inf2(true,"11");
    SSLInfo inf(inf2.toBSONObj().getField("SSL"));
    ASSERT_EQ(inf, inf2);
    
    SSLInfo inf4(false,"");
    SSLInfo inf3(inf4.toBSONObj().getField("SSL"));
    ASSERT_EQ(inf3, inf4);
    
    SSLInfo inf6(false,"");
    SSLInfo inf5(inf6.toBSONObj().getField("SSL"));
    ASSERT_EQ(inf5, inf6);
    
    SSLInfo inf8(true,"");
    SSLInfo inf7(inf8.toBSONObj().getField("SSL"));
    ASSERT_EQ(inf7, inf8);
}

TEST(HostAndPortString, ConnectionString) 
{
    mongo::HostAndPort inf("localhost:11");
    mongo::HostAndPort inf2("localhost",11);
    ASSERT_EQ(inf, inf2);

    SSLInfo ssl(true,"12");
    mongo::HostAndPort inf3("localhost:11"+ssl.toBSONObj().toString());    
    mongo::HostAndPort inf4("localhost",11,ssl,SSHInfo());
    ASSERT_EQ(inf3, inf4);

    SSHInfo ssh("localhost",22,"sasha","lalala",PublicKey(),SSHInfo::PASSWORD);
    mongo::HostAndPort inf5("localhost:11" + ssl.toBSONObj().toString() + ssh.toBSONObj().toString());
    mongo::HostAndPort inf6("localhost",11,ssl,ssh);
    ASSERT_EQ(inf5, inf6);

    mongo::HostAndPort inf7("localhost:11"+SSLInfo().toBSONObj().toString()+SSHInfo().toBSONObj().toString());
    mongo::HostAndPort inf8("localhost",11,SSLInfo(),SSHInfo());
    ASSERT_EQ(inf5, inf6);
        
    mongo::HostAndPort inf9("192.168.38.137:22"+SSLInfo().toBSONObj().toString() + SSHInfo("192.168.38.137",27017,"sasha","1023224",PublicKey(),SSHInfo::PASSWORD).toBSONObj().toString());
    mongo::HostAndPort inf10("192.168.38.137",22,SSLInfo(),SSHInfo("192.168.38.137",27017,"sasha","1023224",PublicKey(),SSHInfo::PASSWORD));
    ASSERT_EQ(inf9, inf10);
}