#include <gtest/gtest.h>
#include <sharp/TypeSet/TypeSet.hpp>

using namespace sharp;

template <typename Tag>
class TestConstruct {
public:
    static int number_default_constructs;
    static int number_move_constructs;
    static int number_copy_constructs;
    static int number_destructs;
    static void reset() {
        number_default_constructs = 0;
        number_move_constructs = 0;
        number_copy_constructs = 0;
    }

    TestConstruct() {
        ++number_default_constructs;
    }
    TestConstruct(const TestConstruct&) {
        ++number_copy_constructs;
    }
    TestConstruct(TestConstruct&&) {
        ++number_move_constructs;
    }
    ~TestConstruct() {
        ++ number_destructs;
    }
};

template <typename Tag>
int TestConstruct<Tag>::number_default_constructs = 0;
template <typename Tag>
int TestConstruct<Tag>::number_move_constructs = 0;
template <typename Tag>
int TestConstruct<Tag>::number_copy_constructs = 0;
template <typename Tag>
int TestConstruct<Tag>::number_destructs = 0;

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
