/**
 * @file test.cpp
 * @author Aaryaman Sagar (rmn100@gmail.com)
 *
 * Includes all the files in the TypeTraits.hpp header and runs static unit
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
#include <iostream>

#include <gtest/gtest.h>
#include <sharp/Traits/Traits.hpp>

using namespace sharp;

TEST(TypeTraits, for_each_tuple) {
    auto vec = std::vector<std::type_index>{typeid(int), typeid(char)};
    auto tup = std::make_tuple(1, 'a');
    auto result_vec = std::vector<std::type_index>{};

    for_each_tuple(tup, [&](auto thing) {
        std::cout << thing << std::endl;
        result_vec.push_back(typeid(decltype(thing)));
    });

    EXPECT_TRUE(std::equal(result_vec.begin(), result_vec.end(), vec.begin()));
}
