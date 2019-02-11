#include "gtest/gtest.h"
#include "robomongo/utils/string_operations.h"

#include <string>

/* Example Test:
 *
 * TEST( [Test_Case_Name], [Test_Name] )
 * TEST( [Test_Case_Name], [UnitOfWorkName_ScenarioUnderTest_ExpectedBehavior] )
 * TEST( StringParserTests, NumberLeftOf_StringWithoutNumber_ReturnsFalse) {
    // ...
   }
*/

TEST(basic_tests, captilizeFirstChar)
{
    // EXPECT_EQ("Abcc", Robomongo::captilizeFirstChar("abc")); // Simulating failing test
    EXPECT_EQ("Abc", Robomongo::captilizeFirstChar("abc")); // Simulating passing test
}

TEST(adv_tests, test_1)
{    
    EXPECT_EQ(42, 42); // Dummy test, delete later
}

TEST(adv_tests, test_2)
{    
    EXPECT_EQ(42, 42); // Dummy test, delete later
}