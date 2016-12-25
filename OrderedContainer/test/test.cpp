#include <sharp/OrderedContainer/OrderedContainer.hpp>
#include <gtest/gtest.h>

#include <vector>
#include <functional>

using namespace sharp;
using namespace std;

TEST(OrderedContainer, simple_test) {
    OrderedContainer<vector<int>, std::less<void>> oc;
    oc.insert(0);
    oc.insert(1);
    oc.insert(2);
    EXPECT_NE(oc.find(0), oc.end());
    EXPECT_EQ(*oc.find(0), 0);
    EXPECT_NE(oc.find(1), oc.end());
    EXPECT_EQ(*oc.find(1), 1);
    EXPECT_NE(oc.find(2), oc.end());
    EXPECT_EQ(*oc.find(2), 2);
}
