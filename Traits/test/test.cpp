/**
 * @file test.cpp
 * @author Aaryaman Sagar (rmn100@gmail.com)
 *
 * Includes all the files in the Traits.hpp header and runs static unit
 * tests on them.  To run all tests simply do the following
 *
 *  g++ -std=c++14 -Wall test.cpp -c -o /dev/null
 *
 * And all the crap object file output will go to /dev/null and you will see
 * the stderr reported.
 */
#include <vector>
#include <typeindex>
#include <tuple>
#include <algorithm>
#include <utility>

#include <gtest/gtest.h>
#include <sharp/Traits/Traits.hpp>

using namespace sharp;

template <typename Tag>
class TestConstructionAlert {
public:
    static int number_default_constructs;
    static int number_move_constructs;
    static int number_copy_constructs;
    static void reset() {
        number_default_constructs = 0;
        number_move_constructs = 0;
        number_copy_constructs = 0;
    }

    TestConstructionAlert() {
        ++number_default_constructs;
    }
    TestConstructionAlert(const TestConstructionAlert&) {
        ++number_copy_constructs;
    }
    TestConstructionAlert(TestConstructionAlert&&) {
        ++number_move_constructs;
    }
};

template <typename Tag>
int TestConstructionAlert<Tag>::number_default_constructs = 0;
template <typename Tag>
int TestConstructionAlert<Tag>::number_move_constructs = 0;
template <typename Tag>
int TestConstructionAlert<Tag>::number_copy_constructs = 0;

TEST(Traits, ForEach) {
    auto vec = std::vector<std::type_index>{typeid(int), typeid(double)};
    auto result_vec = decltype(vec){};

    ForEach<std::tuple<int, double>>{}([&](auto type_context) {
        result_vec.push_back(typeid(typename decltype(type_context)::type));
    });

    EXPECT_TRUE(std::equal(vec.begin(), vec.end(), result_vec.begin()));
}
