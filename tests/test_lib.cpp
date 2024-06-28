#include <gtest/gtest.h>
#include "lib.hpp"

TEST(Greeter, GreeterReturnsHello) {
    EXPECT_EQ(greeter(), "Hello, World!");
}
