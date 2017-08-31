#include <sharp/Overload/Overload.hpp>

#include <gtest/gtest.h>


TEST(Overload, BasicOverloadTest) {
    auto overloaded = sharp::make_overload(
        [](int a) { return a; },
        [](double d) { return d; });

    EXPECT_EQ(overloaded(1), 1);
    EXPECT_EQ(overloaded(2.1), 2.1);
}
