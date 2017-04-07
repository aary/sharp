#include <gtest/gtest.h>
#include <sharp/Defer/Defer.hpp>

TEST(Defer, basic_test) {
    auto is_set = false;
    {
        auto deferred = defer([&is_set]() {
            is_set = true;
        });
    }
    EXPECT_TRUE(is_set);
}
