#include "gtest/gtest.h"
#include "HexUtils.h"

#include <string>

/* Example Test:
*
* TEST( [Test_Case_Name], [Test_Name] )
* TEST( [Test_Case_Name], [UnitOfWorkName_ScenarioUnderTest_ExpectedBehavior] )
* TEST( StringParserTests, NumberLeftOf_StringWithoutNumber_ReturnsFalse) {
// ...
}
*/

TEST(hex_utils_tests, test_1)
{
    EXPECT_TRUE(Robomongo::HexUtils::isHexString("a"));
}

