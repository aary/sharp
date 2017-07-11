#include <sharp/Defer/Defer.hpp>

#include <gtest/gtest.h>

TEST(Defer, basic_test) {
    auto is_set = false;
    {
        auto deferred = sharp::defer([&is_set]() {
            is_set = true;
        });
    }
    EXPECT_TRUE(is_set);
}
