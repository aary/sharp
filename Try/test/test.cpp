#include <sharp/Try/Try.hpp>

#include <gtest/gtest.h>

TEST(Try, DefaultConstruct) {
    auto t = sharp::Try<int>{};
    EXPECT_FALSE(t.has_value());
    EXPECT_FALSE(t.has_exception());
    EXPECT_FALSE(t.valid());
    EXPECT_FALSE(t.is_ready());
    EXPECT_FALSE(t);
}

TEST(Try, NullPtrConstruct) {
    auto t = sharp::Try<int>{nullptr};
    EXPECT_FALSE(t.has_value());
    EXPECT_FALSE(t.has_exception());
    EXPECT_FALSE(t.valid());
    EXPECT_FALSE(t.is_ready());
    EXPECT_FALSE(t);
}
