#include <sharp/Range/Range.hpp>
#include <gtest/gtest.h>

#include <iostream>
#include <limits>
#include <vector>
#include <algorithm>

using namespace std;
using namespace sharp;

TEST(Range, BasicTest) {

    // generate a vector with elements in [0, 10000)
    auto upper_limit = 10000;
    std::vector<int> vec;
    vec.resize(upper_limit);
    std::generate(vec.begin(), vec.end(), [i = 0] () mutable { return i++; });

    // now do the same thing with the range() function
    std::vector<int> vec_two;
    for (auto i : range(0, upper_limit)) {
        vec_two.push_back(i);
    }

    EXPECT_TRUE(std::equal(vec.begin(), vec.end(), vec_two.begin()));
}

TEST(Range, ActualRange) {

    auto v = std::vector<int>{1, 2, 3};
    auto element_counter = 1;
    for (auto i : range(v.begin(), v.end())) {
        EXPECT_EQ(i, element_counter++);
    }
}
