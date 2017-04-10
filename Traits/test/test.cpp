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

TEST(Traits, for_each_tuple_simple_unary) {
    auto vec = std::vector<std::type_index>{typeid(int), typeid(char)};
    auto tup = std::make_tuple(1, 'a');
    auto result_vec = std::vector<std::type_index>{};

    for_each_tuple(tup, [&](auto thing) {
        result_vec.push_back(typeid(decltype(thing)));
    });

    EXPECT_TRUE(std::equal(result_vec.begin(), result_vec.end(), vec.begin()));
}

TEST(Traits, for_each_tuple_simple_unary_reference) {
    auto vec = std::vector<std::type_index>{typeid(int), typeid(char)};
    auto tup = std::make_tuple(1, 'a');
    auto result_vec = std::vector<std::type_index>{};

    for_each_tuple(tup, [&](auto& thing) {
        result_vec.push_back(typeid(decltype(thing)));
    });

    EXPECT_TRUE(std::equal(result_vec.begin(), result_vec.end(), vec.begin()));
}

// TEST(Traits, for_each_tuple_simple_binary) {
    // auto vec = std::vector<std::pair<std::type_index, int>>{
        // {typeid(int), 0},
        // {typeid(char), 1}};
    // auto tup = std::make_tuple(1, 'a');
    // auto result_vec = std::vector<std::pair<std::type_index, int>>{};

    // for_each_tuple(tup, [&](auto thing, int index) {
        // result_vec.push_back({typeid(decltype(thing)), index});
    // });

    // EXPECT_TRUE(std::equal(result_vec.begin(), result_vec.end(), vec.begin()));
// }

TEST(Traits, for_each_tuple_forwarding_unary) {
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

// TEST(Traits, for_each_tuple_forwarding_binary) {
    // TestConstructionAlert<int>::reset();
    // TestConstructionAlert<double>::reset();
    // auto vec = std::vector<std::pair<std::type_index, int>>{
        // {typeid(TestConstructionAlert<int>), 0},
        // {typeid(TestConstructionAlert<double>), 1}};
    // auto tup = std::make_tuple(
            // TestConstructionAlert<int>{},
            // TestConstructionAlert<double>{});
    // EXPECT_EQ(TestConstructionAlert<int>::number_move_constructs, 1);
    // EXPECT_EQ(TestConstructionAlert<double>::number_move_constructs, 1);
    // auto result_vec = decltype(vec){};

    // // iterate through the tuple and make sure that everything is right
    // for_each_tuple(std::move(tup), [&](auto item, int index) {
        // result_vec.push_back({typeid(decltype(item)), index});
    // });

    // EXPECT_TRUE(std::equal(result_vec.begin(), result_vec.end(), vec.begin()));
    // EXPECT_EQ(TestConstructionAlert<int>::number_move_constructs, 2);
    // EXPECT_EQ(TestConstructionAlert<int>::number_copy_constructs, 0);
    // EXPECT_EQ(TestConstructionAlert<int>::number_default_constructs, 1);
    // EXPECT_EQ(TestConstructionAlert<double>::number_move_constructs, 2);
    // EXPECT_EQ(TestConstructionAlert<double>::number_copy_constructs, 0);
    // EXPECT_EQ(TestConstructionAlert<double>::number_default_constructs, 1);
// }

TEST(Traits, ForEach) {
    auto vec = std::vector<std::type_index>{typeid(int), typeid(double)};
    auto result_vec = decltype(vec){};

    ForEach<std::tuple<int, double>>{}([&](auto type_context) {
        result_vec.push_back(typeid(typename decltype(type_context)::type));
    });

    EXPECT_TRUE(std::equal(vec.begin(), vec.end(), result_vec.begin()));
}

TEST(Traits, match_forward_lvalue_to_lvalue) {

    int to_cast;

    // try casting when both of the things are lvalues
    auto&& one = sharp::match_forward<int&, int&>(to_cast);
    static_assert(std::is_lvalue_reference<decltype(one)>::value, "");

    // try casting when the first thing is an rvalue and the second is an
    // lvalue, rvalues come in two forms when using forwarding references
    //
    //  template <typename Something>
    //  void something(Something&&)
    //
    // Here if the thing passed to the function something() is an rvalue the
    // type of it will be deduced to std::remove_reference<Something> so when
    // you forward with a call like this
    //
    //  sharp::match_forward<Something, Else>(else);
    //
    // The type that you will get will be std::remove_reference<Something>
    // with no reference qualifiers.
    //
    // The other possibility is when the user uses auto in a deduced context
    //
    //  [](auto&& something) { ... }
    //
    // Here the only thing they can do to forward something else with the
    // value type of something is to use decltype(something) and that
    // evaluates to std::remove_reference_t<decltype(something)>&&
    //
    // Therefore match forward should be able to match the rvalue-ness of the
    // first type in both the cases above, when there are two reference signs
    // and when there are not
    auto&& two = sharp::match_forward<int&&, int&>(to_cast);
    static_assert(std::is_rvalue_reference<decltype(two)>::value, "");
    auto&& three = sharp::match_forward<int, int&>(to_cast);
    static_assert(std::is_rvalue_reference<decltype(three)>::value, "");

    // now try casting when the second thing is an rvalue, represented both by
    // no reference qualifier above as well as two reference qualifiers,
    // the reasoning for which is explained above
    //
    // The first case when the first thing is an lvalue should not compile
    // becasue forwarding an rvalue as an lvalue should be ill formed as it is
    // prone to errors
    auto&& four = sharp::match_forward<int, int&&>(int{});
    static_assert(std::is_rvalue_reference<decltype(four)>::value, "");
    auto&& five = sharp::match_forward<int&&, int&&>(int{});
    static_assert(std::is_rvalue_reference<decltype(five)>::value, "");

    auto&& six = sharp::match_forward<int, int>(int{});
    static_assert(std::is_rvalue_reference<decltype(six)>::value, "");
    auto&& seven = sharp::match_forward<int&&, int>(int{});
    static_assert(std::is_rvalue_reference<decltype(seven)>::value, "");
}
