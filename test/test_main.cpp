////////////////////////////////////////////////////////////
//  test main file
//  implements the test entry point
//
// Written : 15/01/2025
////////////////////////////////////////////////////////////

#include <gtest/gtest.h>

int main(int argc, char **argv) {
    testing::FLAGS_gtest_death_test_style = "threadsafe";
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
