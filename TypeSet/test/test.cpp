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
