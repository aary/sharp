#include <sharp/Singleton/Singleton.hpp>

#include <gtest/gtest.h>

TEST(Singleton, simple_test_1) {
    auto integer_singleton_one = sharp::Singleton<int>::get_strong();
    auto integer_singleton_two = sharp::Singleton<int>::get_strong();
    EXPECT_EQ(integer_singleton_one.get(), integer_singleton_two.get());
}
