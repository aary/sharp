#include <vector>
#include <typeindex>

#include <gtest/gtest.h>
#include <sharp/TypeSet/TypeSet.hpp>

using namespace std;
using namespace sharp;

namespace {
    vector<type_index> order_default_constructs;
    vector<type_index> order_move_constructs;
    vector<type_index> order_copy_constructs;
    vector<type_index> order_copy_assigns;
    vector<type_index> order_move_assigns;
    vector<type_index> order_destructs;
}

template <typename Tag>
class TestConstruct {
public:
    static int number_default_constructs;
    static int number_move_constructs;
    static int number_copy_constructs;
    static int number_copy_assigns;
    static int number_move_assigns;
    static int number_destructs;

    static void reset() {
        number_default_constructs = 0;
        number_move_constructs = 0;
        number_copy_constructs = 0;
        number_move_assigns = 0;
        number_copy_assigns = 0;
        number_destructs = 0;
        order_default_constructs.clear();
        order_move_constructs.clear();
        order_copy_constructs.clear();
        order_destructs.clear();
    }

    TestConstruct() {
        ++number_default_constructs;
        order_default_constructs.push_back(typeid(Tag));
    }
    TestConstruct(const TestConstruct&) {
        ++number_copy_constructs;
        order_copy_constructs.push_back(typeid(Tag));
    }
    TestConstruct(TestConstruct&&) {
        ++number_move_constructs;
        order_move_constructs.push_back(typeid(Tag));
    }
    TestConstruct& operator=(const TestConstruct&) {
        ++number_copy_assigns;
        order_copy_assigns.push_back(typeid(Tag));
        return *this;
    }
    TestConstruct& operator=(TestConstruct&&) {
        ++number_move_assigns;
        order_move_assigns.push_back(typeid(Tag));
        return *this;
    }
    ~TestConstruct() {
        ++number_destructs;
        order_destructs.push_back(typeid(Tag));
    }

    Tag datum;
};

template <typename Tag>
int TestConstruct<Tag>::number_default_constructs = 0;
template <typename Tag>
int TestConstruct<Tag>::number_move_constructs = 0;
template <typename Tag>
int TestConstruct<Tag>::number_copy_constructs = 0;
template <typename Tag>
int TestConstruct<Tag>::number_destructs = 0;
template <typename Tag>
int TestConstruct<Tag>::number_copy_assigns = 0;
template <typename Tag>
int TestConstruct<Tag>::number_move_assigns = 0;

TEST(TypeSet, construct_test) {
    TestConstruct<int>::reset();
    TestConstruct<double>::reset();
    {
        TypeSet<TestConstruct<int>, TestConstruct<double>> ts;
    }
    EXPECT_EQ(TestConstruct<int>::number_default_constructs, 1);
    EXPECT_EQ(TestConstruct<int>::number_destructs, 1);
    EXPECT_EQ(TestConstruct<double>::number_default_constructs, 1);
    EXPECT_EQ(TestConstruct<double>::number_destructs, 1);
}

TEST(TypeSet, construct_test_order) {
    TestConstruct<int>::reset();
    TestConstruct<double>::reset();
    {
        TypeSet<TestConstruct<int>, TestConstruct<double>> ts;
    }
    auto order_constructs_correct = vector<type_index>{typeid(int),
                                                       typeid(double)};
    auto order_destructs_correct = vector<type_index>{typeid(int),
                                                      typeid(double)};
    EXPECT_TRUE(std::equal(order_constructs_correct.begin(),
                           order_constructs_correct.end(),
                           order_default_constructs.begin()));
    EXPECT_TRUE(std::equal(order_destructs_correct.begin(),
                           order_destructs_correct.end(),
                           order_destructs.begin()));
}

TEST(TypeSet, get_right_type) {
    TestConstruct<int>::reset();
    TestConstruct<double>::reset();

    {
        TypeSet<TestConstruct<int>, TestConstruct<double>> ts;
        sharp::get<TestConstruct<int>>(ts) = TestConstruct<int>{};
        sharp::get<TestConstruct<double>>(ts) = TestConstruct<double>{};
    }

    EXPECT_EQ(TestConstruct<int>::number_default_constructs, 2);
    EXPECT_EQ(TestConstruct<int>::number_move_assigns, 1);
    EXPECT_EQ(TestConstruct<int>::number_destructs, 2);

    EXPECT_EQ(TestConstruct<double>::number_default_constructs, 2);
    EXPECT_EQ(TestConstruct<double>::number_move_assigns, 1);
    EXPECT_EQ(TestConstruct<double>::number_destructs, 2);
}

TEST(TypeSet, get_lvalue_forward) {
    TestConstruct<int>::reset();
    TestConstruct<double>::reset();

    TypeSet<TestConstruct<int>, TestConstruct<double>> one;
    static_assert(std::is_same<decltype(sharp::get<TestConstruct<int>>(one)),
                               TestConstruct<int>&>::value, "Failed!");
    static_assert(std::is_same<decltype(sharp::get<TestConstruct<double>>(one)),
                               TestConstruct<double>&>::value, "Failed!");

    const TypeSet<TestConstruct<int>, TestConstruct<double>> two;
    static_assert(std::is_same<decltype(sharp::get<TestConstruct<int>>(two)),
                               const TestConstruct<int>&>::value, "Failed!");
    static_assert(std::is_same<decltype(sharp::get<TestConstruct<double>>(two)),
                             const TestConstruct<double>&>::value, "Failed!");
}

TEST(TypeSet, get_rvalue_forward) {
    TestConstruct<int>::reset();
    TestConstruct<double>::reset();

    TypeSet<TestConstruct<int>, TestConstruct<double>> one;
    static_assert(std::is_same<
            decltype(sharp::get<TestConstruct<int>>(std::move(one))),
            TestConstruct<int>&&>::value, "Failed!");
    static_assert(std::is_same<
            decltype(sharp::get<TestConstruct<double>>(std::move(one))),
            TestConstruct<double>&&>::value, "Failed!");

    const TypeSet<TestConstruct<int>, TestConstruct<double>> two;
    static_assert(std::is_same<
            decltype(sharp::get<TestConstruct<int>>(std::move(two))),
            const TestConstruct<int>&&>::value, "Failed!");
    static_assert(std::is_same<
            decltype(sharp::get<TestConstruct<double>>(std::move(two))),
            const TestConstruct<double>&&>::value, "Failed!");
}

TEST(TypeSet, type_exists) {
    TypeSet<int, double> ts;
    EXPECT_TRUE(decltype(ts)::exists<int>());
    EXPECT_TRUE(decltype(ts)::exists<double>());
    EXPECT_TRUE(decltype(ts)::exists<float>());
}

TEST(TypeSet, test_collect_args_explicit_types) {
    TestConstruct<int>::reset();
    TestConstruct<double>::reset();

    // try with another function
    auto func = [](auto&&... args) {
        auto args_set = sharp::collect_args<int, double>(
                std::forward<decltype(args)>(args)...);
        static_assert(std::is_same<decltype(args_set),
                                   sharp::TypeSet<int, double>>::value, "Bad!");
    };
    func(1, 1.2);

    sharp::collect_args<TestConstruct<int>, TestConstruct<double>>(
            TestConstruct<int>{}, TestConstruct<double>{});

    auto vec_expected_move_constructs = std::vector<type_index>{typeid(int),
                                                                typeid(double)};
    EXPECT_EQ(TestConstruct<int>::number_default_constructs, 1);
    EXPECT_EQ(TestConstruct<int>::number_destructs, 2);
    EXPECT_EQ(TestConstruct<int>::number_move_constructs, 1);
    EXPECT_TRUE(std::equal(order_move_constructs.begin(),
            order_move_constructs.end(), vec_expected_move_constructs.begin()));
    EXPECT_EQ(TestConstruct<double>::number_default_constructs, 1);
    EXPECT_EQ(TestConstruct<double>::number_destructs, 2);
    EXPECT_EQ(TestConstruct<double>::number_move_constructs, 1);
}

TEST(TypeSet, test_collect_args_explicit_types_out_of_order) {
    TestConstruct<int>::reset();
    TestConstruct<double>::reset();

    sharp::collect_args<TestConstruct<int>, TestConstruct<double>>(
            TestConstruct<double>{}, TestConstruct<int>{});

    auto vec_expected_move_constructs = std::vector<type_index>{typeid(int),
                                                                typeid(double)};
    EXPECT_EQ(TestConstruct<int>::number_default_constructs, 1);
    EXPECT_EQ(TestConstruct<int>::number_destructs, 2);
    EXPECT_EQ(TestConstruct<int>::number_move_constructs, 1);
    EXPECT_TRUE(std::equal(order_move_constructs.begin(),
            order_move_constructs.end(), vec_expected_move_constructs.begin()));
    EXPECT_EQ(TestConstruct<double>::number_default_constructs, 1);
    EXPECT_EQ(TestConstruct<double>::number_destructs, 2);
    EXPECT_EQ(TestConstruct<double>::number_move_constructs, 1);
}

TEST(TypeSet, test_copy_assign) {
    TestConstruct<int>::reset();
    TestConstruct<double>::reset();

    TypeSet<TestConstruct<int>, TestConstruct<double>> ts;
    sharp::get<TestConstruct<int>>(ts) = TestConstruct<int>{};
    sharp::get<TestConstruct<double>>(ts) = TestConstruct<double>{};

    EXPECT_EQ(TestConstruct<int>::number_default_constructs, 2);
    EXPECT_EQ(TestConstruct<double>::number_default_constructs, 2);
    EXPECT_EQ(TestConstruct<int>::number_move_assigns, 1);
    EXPECT_EQ(TestConstruct<double>::number_move_assigns, 1);

    TypeSet<TestConstruct<int>, TestConstruct<double>> ts_two;
    ts = ts_two;

    EXPECT_EQ(TestConstruct<int>::number_default_constructs, 3);
    EXPECT_EQ(TestConstruct<int>::number_copy_assigns, 1);
    EXPECT_EQ(TestConstruct<double>::number_default_constructs, 3);
    EXPECT_EQ(TestConstruct<double>::number_copy_assigns, 1);
}

TEST(TypeSet, test_move_assign) {
    TestConstruct<int>::reset();
    TestConstruct<double>::reset();

    TypeSet<TestConstruct<int>, TestConstruct<double>> one;
    TypeSet<TestConstruct<int>, TestConstruct<double>> two;

    one = std::move(two);

    EXPECT_EQ(TestConstruct<int>::number_default_constructs, 2);
    EXPECT_EQ(TestConstruct<double>::number_default_constructs, 2);
    EXPECT_EQ(TestConstruct<int>::number_move_assigns, 1);
    EXPECT_EQ(TestConstruct<double>::number_move_assigns, 1);
}
