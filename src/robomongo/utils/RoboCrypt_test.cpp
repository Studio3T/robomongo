#include "gtest/gtest.h"
#include "robomongo/utils/RoboCrypt.h"

#include <string>

/* Example Test:
 *
 * TEST( [Test_Case_Name], [Test_Name] )
 * TEST( [Test_Case_Name], [UnitOfWorkName_ScenarioUnderTest_ExpectedBehavior] )
 * TEST( StringParserTests, NumberLeftOf_StringWithoutNumber_ReturnsFalse) {
    // ...
   }
*/

TEST(RoboCrypt_BasicTests, encrypt_decrypt)
{  
    const std::string pwd {"abc"};
    const std::string encryptedPwd = Robomongo::RoboCrypt::encrypt(pwd);
    const std::string decryptedPwd = Robomongo::RoboCrypt::decrypt(encryptedPwd);
    EXPECT_EQ(pwd, decryptedPwd);
}