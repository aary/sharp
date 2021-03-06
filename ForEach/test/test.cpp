#include <sharp/ForEach/ForEach.hpp>

#include <gtest/gtest.h>

#include <vector>
#include <typeindex>
#include <tuple>
#include <algorithm>
#include <utility>
#include <list>

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

TEST(ForEach, simple_runtime_range) {
    auto range = std::vector<int>{1, 2, 3};
    auto iterations = 0;
    sharp::for_each(range, [&](auto ele) {
        ++iterations;
        EXPECT_EQ(iterations, ele);
    });
    EXPECT_EQ(iterations, 3);
}

TEST(ForEach, simple_runtime_range_breakable) {
    auto range = std::vector<int>{1, 2, 3};
    auto iterations = 0;
    sharp::for_each(range, [&](auto ele) {
        ++iterations;
        EXPECT_EQ(iterations, ele);
        if (iterations == 2) {
            return sharp::loop_break;
        }
        return sharp::loop_continue;
    });
    EXPECT_EQ(iterations, 2);
}

TEST(ForEach, simple_tuple_range) {
    auto range = std::make_tuple(1, 2, 3);
    auto iterations = 0;
    sharp::for_each(range, [&](auto ele) {
        ++iterations;
        EXPECT_EQ(iterations, ele);
    });
    EXPECT_EQ(iterations, 3);
}

TEST(ForEach, simple_tuple_range_breakable) {
    auto range = std::make_tuple(1, 2, 3);
    auto iterations = 0;
    sharp::for_each(range, [&](auto ele) {
        ++iterations;
        EXPECT_EQ(iterations, ele);
        if (iterations == 2) {
            return sharp::loop_break;
        }
        return sharp::loop_continue;
    });
    EXPECT_EQ(iterations, 2);
}

// TEST(ForEach, simple_test) {
    // auto range = std::vector<int>{};
    // range.resize(1e9);
    // range.back() = -1;
    // sharp::for_each(range, [](volatile auto ele) {
        // if (ele == -1) {
            // return sharp::loop_break;
        // }
        // return sharp::loop_continue;
    // });
    // for (volatile auto ele : range) {
        // if (ele == -1) {
            // break;
        // }
    // }
// }

TEST(ForEach, for_each_simple_unary) {
    auto vec = std::vector<std::type_index>{typeid(int), typeid(char)};
    auto tup = std::make_tuple(1, 'a');
    auto result_vec = std::vector<std::type_index>{};

    sharp::for_each(tup, [&](auto thing) {
        result_vec.push_back(typeid(decltype(thing)));
    });

    EXPECT_TRUE(std::equal(result_vec.begin(), result_vec.end(), vec.begin()));
}

TEST(ForEach, for_each_simple_unary_reference) {
    auto vec = std::vector<std::type_index>{typeid(int), typeid(char)};
    auto tup = std::make_tuple(1, 'a');
    auto result_vec = std::vector<std::type_index>{};

    sharp::for_each(tup, [&](auto& thing) {
        result_vec.push_back(typeid(decltype(thing)));
    });

    EXPECT_TRUE(std::equal(result_vec.begin(), result_vec.end(), vec.begin()));
}

TEST(ForEach, for_each_simple_binary) {
    auto vec = std::vector<std::pair<std::type_index, int>>{
        {typeid(int), 0},
        {typeid(char), 1}};
    auto tup = std::make_tuple(1, 'a');
    auto result_vec = std::vector<std::pair<std::type_index, int>>{};

    sharp::for_each(tup, [&](auto thing, auto index) {
        result_vec.push_back({typeid(decltype(thing)),
            static_cast<int>(index)});
    });

    EXPECT_TRUE(std::equal(result_vec.begin(), result_vec.end(), vec.begin()));
}

TEST(ForEach, for_each_forwarding_unary) {
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
    sharp::for_each(std::move(tup), [&](auto item) {
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

TEST(ForEach, for_each_forwarding_binary) {
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
    sharp::for_each(std::move(tup), [&](auto item, auto index) {
        result_vec.push_back({typeid(decltype(item)), static_cast<int>(index)});
    });

    EXPECT_TRUE(std::equal(result_vec.begin(), result_vec.end(), vec.begin()));
    EXPECT_EQ(TestConstructionAlert<int>::number_move_constructs, 2);
    EXPECT_EQ(TestConstructionAlert<int>::number_copy_constructs, 0);
    EXPECT_EQ(TestConstructionAlert<int>::number_default_constructs, 1);
    EXPECT_EQ(TestConstructionAlert<double>::number_move_constructs, 2);
    EXPECT_EQ(TestConstructionAlert<double>::number_copy_constructs, 0);
    EXPECT_EQ(TestConstructionAlert<double>::number_default_constructs, 1);
}

TEST(ForEach, for_each_runtime_unary) {
    auto v = std::vector<int>{1, 2, 4, 8, 16};
    auto counter = 1;
    sharp::for_each(v, [&](auto integer) {
        EXPECT_EQ(counter, integer);
        counter *= 2;
    });
}

TEST(ForEach, for_each_runtime_binary) {
    auto v = std::vector<int>{1, 2, 4, 8, 16};
    auto element_counter = 1;
    auto index_counter = std::size_t{0};
    sharp::for_each(v, [&](auto integer, auto index) {
        EXPECT_EQ(element_counter, integer);
        EXPECT_EQ(index_counter, index);
        element_counter *= 2;
        ++index_counter;
    });
}

TEST(ForEach, fetch_test) {
    auto tup = std::make_tuple(1, 2);
    auto one = sharp::fetch(tup, std::integral_constant<int, 0>{});
    auto two = sharp::fetch(tup, std::integral_constant<int, 1>{});
    EXPECT_EQ(one, 1);
    EXPECT_EQ(two, 2);
    one = 3;
    EXPECT_EQ(one, 3);

    auto vec = std::vector<int>{3, 4};
    auto three = sharp::fetch(vec, std::integral_constant<int, 0>{});
    auto four = sharp::fetch(vec, std::integral_constant<int, 1>{});
    EXPECT_EQ(three, 3);
    EXPECT_EQ(four, 4);
}
