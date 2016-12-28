#include <sharp/Range/Range.hpp>
#include <gtest/gtest.h>

#include <iostream>
#include <limits>
#include <vector>
#include <algorithm>

using namespace std;
using namespace sharp;

TEST(Range, BasicTest) {

    auto upper_limit = 10000;
    std::vector<int> vec;
    vec.resize(upper_limit);
    std::generate(vec.begin(), vec.end(), [i = 0] () mutable { return i++; });

    std::vector<int> vec_two;
    for (auto i : range(0, upper_limit)) {
        vec_two.push_back(i);
    }

    EXPECT_TRUE(std::equal(vec.begin(), vec.end(), vec_two.begin()));
}
