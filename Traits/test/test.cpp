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

TEST(TypeTraits, for_each_tuple_simple_unary) {
    auto vec = std::vector<std::type_index>{typeid(int), typeid(char)};
    auto tup = std::make_tuple(1, 'a');
    auto result_vec = std::vector<std::type_index>{};

    for_each_tuple(tup, [&](auto thing) {
        result_vec.push_back(typeid(decltype(thing)));
    });

    EXPECT_TRUE(std::equal(result_vec.begin(), result_vec.end(), vec.begin()));
}

TEST(TypeTraits, for_each_tuple_simple_unary_reference) {
    auto vec = std::vector<std::type_index>{typeid(int), typeid(char)};
    auto tup = std::make_tuple(1, 'a');
    auto result_vec = std::vector<std::type_index>{};

    for_each_tuple(tup, [&](auto& thing) {
        result_vec.push_back(typeid(decltype(thing)));
    });

    EXPECT_TRUE(std::equal(result_vec.begin(), result_vec.end(), vec.begin()));
}

TEST(TypeTraits, for_each_tuple_simple_binary) {
    auto vec = std::vector<std::pair<std::type_index, int>>{
        {typeid(int), 0},
        {typeid(char), 1}};
    auto tup = std::make_tuple(1, 'a');
    auto result_vec = std::vector<std::pair<std::type_index, int>>{};

    for_each_tuple(tup, [&](auto thing, int index) {
        result_vec.push_back({typeid(decltype(thing)), index});
    });

    EXPECT_TRUE(std::equal(result_vec.begin(), result_vec.end(), vec.begin()));
}

TEST(TypeTraits, for_each_tuple_forwarding_unary) {
    TestConstructionAlert<int>::reset();
    TestConstructionAlert<double>::reset();
    auto vec = std::vector<std::type_index>{
        typeid(TestConstructionAlert<int>),
        typeid(TestConstructionAlert<double>)};
    auto tup = std::make_tuple(
            TestConstructionAlert<int>{},
            TestConstructionAlert<double>{});
    EXPECT_EQ(TestConstructionAlert<int>::number_move_constructs, 1);
    EXPECT_EQ(TestConstructionAlert<double>::number_move_constructs, 1);
    auto result_vec = decltype(vec){};

    // iterate through the tuple and make sure that everything is right
    for_each_tuple(std::move(tup), [&](auto item) {
        result_vec.push_back(typeid(decltype(item)));
    });

    EXPECT_TRUE(std::equal(result_vec.begin(), result_vec.end(), vec.begin()));
    EXPECT_EQ(TestConstructionAlert<int>::number_move_constructs, 2);
    EXPECT_EQ(TestConstructionAlert<int>::number_copy_constructs, 0);
    EXPECT_EQ(TestConstructionAlert<int>::number_default_constructs, 1);
    EXPECT_EQ(TestConstructionAlert<double>::number_move_constructs, 2);
    EXPECT_EQ(TestConstructionAlert<double>::number_copy_constructs, 0);
    EXPECT_EQ(TestConstructionAlert<double>::number_default_constructs, 1);
}

TEST(TypeTraits, for_each_tuple_forwarding_binary) {
    TestConstructionAlert<int>::reset();
    TestConstructionAlert<double>::reset();
    auto vec = std::vector<std::pair<std::type_index, int>>{
        {typeid(TestConstructionAlert<int>), 0},
        {typeid(TestConstructionAlert<double>), 1}};
    auto tup = std::make_tuple(
            TestConstructionAlert<int>{},
            TestConstructionAlert<double>{});
    EXPECT_EQ(TestConstructionAlert<int>::number_move_constructs, 1);
    EXPECT_EQ(TestConstructionAlert<double>::number_move_constructs, 1);
    auto result_vec = decltype(vec){};

    // iterate through the tuple and make sure that everything is right
    for_each_tuple(std::move(tup), [&](auto item, int index) {
        result_vec.push_back({typeid(decltype(item)), index});
    });

    EXPECT_TRUE(std::equal(result_vec.begin(), result_vec.end(), vec.begin()));
    EXPECT_EQ(TestConstructionAlert<int>::number_move_constructs, 2);
    EXPECT_EQ(TestConstructionAlert<int>::number_copy_constructs, 0);
    EXPECT_EQ(TestConstructionAlert<int>::number_default_constructs, 1);
    EXPECT_EQ(TestConstructionAlert<double>::number_move_constructs, 2);
    EXPECT_EQ(TestConstructionAlert<double>::number_copy_constructs, 0);
    EXPECT_EQ(TestConstructionAlert<double>::number_default_constructs, 1);
}
