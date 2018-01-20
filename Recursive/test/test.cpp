#include <sharp/Recursive/Recursive.hpp>

#include <gtest/gtest.h>

TEST(RecursiveTest, Basic) {
    auto sum = sharp::recursive([](auto& self, auto start, auto end, auto sum) {
        if (start == end) {
            return sum;
        }

        return self(start + 1, end, sum + start);
    });

    EXPECT_EQ(sum(0, 5, 0), 10);
}
