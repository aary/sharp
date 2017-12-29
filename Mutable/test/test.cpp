#include <sharp/Mutable/Mutable.hpp>
#include <gtest/gtest.h>

namespace sharp {

TEST(Mutable, BasicTest) {
    const auto mutable_integer = sharp::Mutable<int>{1};
    EXPECT_EQ(*mutable_integer, 1);
    *mutable_integer = 2;
    EXPECT_EQ(*mutable_integer, 2);
}

} // namespace sharp
