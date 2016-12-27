#include <sharp/OrderedContainer/OrderedContainer.hpp>
#include <gtest/gtest.h>

#include <vector>
#include <functional>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <iostream>
#include <set>

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

vector<int> generate_random_integers(int number_of_integers) {
    vector<int> random_integers;
    random_integers.reserve(number_of_integers);
    std::srand(std::time(nullptr));
    for (auto i = 0; i < number_of_integers; ++i) {
        random_integers.push_back(std::rand() % 100);
    }
    std::sort(random_integers.begin(), random_integers.end());
    random_integers.erase(
            std::unique(random_integers.begin(), random_integers.end()),
            random_integers.end());
    std::random_shuffle(random_integers.begin(), random_integers.end());

    return random_integers;
}

TEST(OrderedContainer, vector_test) {
    OrderedContainer<vector<int>, std::less<void>> oc;
    auto random_integers = generate_random_integers(10000);

    for (const auto ele : random_integers) {
        oc.insert(ele);
    }
    std::sort(random_integers.begin(), random_integers.end());

    // make sure that all the elements are in the list, pop them one by one
    // out and then assert that the ranges are equal
    for (; !oc.empty();) {
        auto value_to_remove = random_integers.back();
        random_integers.pop_back();

        // erase from the ordered container
        auto iter = oc.find(value_to_remove);
        EXPECT_NE(iter, oc.end());
        oc.erase(iter);

        EXPECT_TRUE(std::equal(oc.begin(), oc.end(), random_integers.begin()));
    }
}
