#include "gtest/gtest.h"
#include "lib.hpp"

TEST(LibTest, AddFunction)
{
    EXPECT_EQ(add(1, 2), 3);
    EXPECT_EQ(add(-1, -1), -2);
}