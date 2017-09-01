#include <sharp/Overload/Overload.hpp>

#include <gtest/gtest.h>

namespace {
    int foo(int a) {
        return a;
    }
    double bar(double d) {
        return d;
    }
} // namespace <anonymous>

TEST(Overload, BasicOverloadTest) {
    auto overloaded = sharp::make_overload(
        [](int a) { return a; },
        [](double d) { return d; });

    EXPECT_EQ(overloaded(1), 1);
    EXPECT_TRUE((std::is_same<decltype(overloaded(1)), int>::value));
    EXPECT_EQ(overloaded(2.1), 2.1);
    EXPECT_TRUE((std::is_same<decltype(overloaded(2.1)), double>::value));
}

TEST(Overload, BasicOverloadTestFunctionPointers) {
    auto overloaded = sharp::make_overload(foo, bar);

    EXPECT_EQ(overloaded(1), 1);
    EXPECT_TRUE((std::is_same<decltype(overloaded(1)), int>::value));
    EXPECT_EQ(overloaded(1.2), 1.2);
    EXPECT_TRUE((std::is_same<decltype(overloaded(1.2)), double>::value));
}

TEST(Overload, FunctionPointerAndFunctor) {
    auto lambda_one = [](char ch) { return ch; };
    auto overloaded = sharp::make_overload(lambda_one, foo, bar);

    EXPECT_EQ(overloaded(1), 1);
    EXPECT_TRUE((std::is_same<decltype(overloaded(1)), int>::value));
    EXPECT_EQ(overloaded(1.2), 1.2);
    EXPECT_TRUE((std::is_same<decltype(overloaded(1.2)), double>::value));
    EXPECT_EQ(overloaded('a'), 'a');
    EXPECT_TRUE((std::is_same<decltype(overloaded('a')), char>::value));
}
