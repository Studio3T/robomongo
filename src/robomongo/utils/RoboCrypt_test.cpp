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

TEST(RoboCrypt_CoreTests, encrypt_decrypt)
{  
    auto const pwds = {
        "Tyu_aBq",
        "_?asdfghjkl;'piop[.,/",
        ".?/`_@~!#$%^^&&*)_)_+=-",        
        "<>?/.,;':][p{}|\""
    };
    for (auto const& pwd : pwds) {
        const std::string encryptedPwd = Robomongo::RoboCrypt::encrypt(pwd);
        const std::string decryptedPwd = Robomongo::RoboCrypt::decrypt(encryptedPwd);
        EXPECT_EQ(pwd, decryptedPwd);
    }
}