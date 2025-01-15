////////////////////////////////////////////////////////////
//  assert test file
//  implements test cases for assert functions
//
// Written : 15/01/2025
////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include <string>

extern "C" {
    #include "assert/xAssert.h"
}

class AssertTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialisation si nécessaire
    }
};

// Test assertion vraie
TEST_F(AssertTest, TrueAssertion) {
    EXPECT_NO_FATAL_FAILURE(X_ASSERT(true));
}

// Test assertion avec message
TEST_F(AssertTest, AssertWithMessage) {
    const char* message = "Test message";
    EXPECT_DEATH(xAssert((const uint8_t*)"test.cpp", 42, message), ".*");
}

// Test assertion fausse avec mode EXIT
#ifdef XOS_ASSERT_MODE_EXIT
TEST_F(AssertTest, FalseAssertionExit) {
    EXPECT_DEATH(X_ASSERT(false), ".*");
}
#endif

// Test X_ASSERT_RETURN avec condition vraie
TEST_F(AssertTest, AssertReturnTrue) {
    int result = 0;
    X_ASSERT_RETURN(true, -1);
    EXPECT_EQ(result, 0);
}

// Test X_ASSERT_RETURN avec condition fausse
TEST_F(AssertTest, AssertReturnFalse) {
    int result = X_ASSERT_RETURN(false, -1);
    EXPECT_EQ(result, -1);
}

// Test xAssertReturn avec différentes valeurs
TEST_F(AssertTest, AssertReturnValues) {
    EXPECT_EQ(xAssertReturn((const uint8_t*)"test.cpp", 42, nullptr, 0), 0);
    EXPECT_EQ(xAssertReturn((const uint8_t*)"test.cpp", 42, nullptr, -1), -1);
    EXPECT_EQ(xAssertReturn((const uint8_t*)"test.cpp", 42, nullptr, 42), 42);
}

// Test assertions avec expressions complexes
TEST_F(AssertTest, ComplexExpressions) {
    int x = 5, y = 10;
    EXPECT_NO_FATAL_FAILURE(X_ASSERT(x < y));
    EXPECT_NO_FATAL_FAILURE(X_ASSERT(x != y));
}

// Test assertions avec effets de bord
TEST_F(AssertTest, SideEffects) {
    int counter = 0;
    EXPECT_NO_FATAL_FAILURE(X_ASSERT(++counter > 0));
    EXPECT_EQ(counter, 1);
}

// Test paramètres NULL
TEST_F(AssertTest, NullParameters) {
    EXPECT_NO_FATAL_FAILURE(xAssert(nullptr, 0, nullptr));
    EXPECT_EQ(xAssertReturn(nullptr, 0, nullptr, 0), 0);
}

// Test assertions multiples
TEST_F(AssertTest, MultipleAssertions) {
    EXPECT_NO_FATAL_FAILURE({
        X_ASSERT(true);
        X_ASSERT(1 == 1);
        X_ASSERT(nullptr == nullptr);
    });
}
