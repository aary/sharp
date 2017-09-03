#include <sharp/Overload/Overload.hpp>

#include <gtest/gtest.h>

using namespace sharp;

namespace {

    /**
     * Some test functors
     */
    class One {
    public:
        void operator()() {}
    };
    class Two {
        int operator()(double) { return 1; }
    };
    void foo() {}
    int bar(int) { return 1; }
    void baz(int) {}

    template <typename...> struct WhichType;

} // namespace <anonymous>

TEST(Overload, BasicFunctorOverloadTest) {
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

TEST(Overload, BasicFunctionOverloadTesT) {
    auto overloaded = sharp::make_overload(foo, bar);
    EXPECT_EQ(overloaded(1), 1);
    EXPECT_TRUE((std::is_same<decltype(overloaded()), void>::value));
}

TEST(Overload, BasicOneFunctorTwoFunctionTest) {
    auto overloaded = sharp::make_overload(
        [](double d) { return d; },
        bar);
    EXPECT_TRUE((std::is_same<decltype(overloaded(1.2)), double>::value));
    EXPECT_EQ(overloaded(1.2), 1.2);
    EXPECT_TRUE((std::is_same<decltype(overloaded(1)), int>::value));
    EXPECT_EQ(overloaded(1), 1);
}

TEST(Overload, BasicOneFunctorOneFunctionTest) {
    auto overloaded = sharp::make_overload([](double d) { return d; }, baz);
    EXPECT_EQ(overloaded(1.2), 1.2);
    EXPECT_TRUE((std::is_same<decltype(overloaded(1)), void>::value));
}

TEST(Overload, TestInternalSplitLists) {

    EXPECT_TRUE((std::is_same<
            typename overload_detail::SplitLists<ValueList<>, ValueList<>, 0,
                                One,
                                void (*) (),
                                Two,
                                int (*) (double)>::type,
            std::pair<ValueList<0, 2>, ValueList<1, 3>>>::value));
}

TEST(Overload, TestInternalSplitArgs) {
    auto one = One{};
    auto two = Two{};
    auto args = std::forward_as_tuple(one, foo, std::move(two), bar);
    auto split_args = overload_detail::SplitFunctorAndFunctionPointers<
        decltype(args)>::impl(args);

    EXPECT_TRUE((std::is_same<decltype(split_args), std::tuple<One&,
                Two&&, void (&) (), int (&) (int)>>::value));
}
