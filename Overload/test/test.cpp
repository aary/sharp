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

    int lvalue(int&) { return 1; }
    int const_lvalue(const int&) { return 2; }
    int rvalue(int&&) { return 3; }
    int const_rvalue(const int&&) { return 4; }

} // namespace <anonymous>

TEST(Overload, BasicFunctorOverloadTest) {
    auto overloaded = sharp::overload(
        [](int a) { return a; },
        [](double d) { return d; });

    EXPECT_EQ(overloaded(1), 1);
    EXPECT_TRUE((std::is_same<decltype(overloaded(1)), int>::value));
    EXPECT_EQ(overloaded(2.1), 2.1);
    EXPECT_TRUE((std::is_same<decltype(overloaded(2.1)), double>::value));
}

TEST(Overload, BasicFunctionOverloadTest) {
    auto overloaded = sharp::overload(foo, bar);
    EXPECT_EQ(overloaded(1), 1);
    EXPECT_TRUE((std::is_same<decltype(overloaded()), void>::value));
}

TEST(Overload, BasicOneFunctorOneFunctionTest) {
    auto overloaded = sharp::overload([](double d) { return d; }, baz);
    EXPECT_EQ(overloaded(1.2), 1.2);
    EXPECT_TRUE((std::is_same<decltype(overloaded(1)), void>::value));
}

TEST(Overload, BasicOneFunctorTwoFunctionTest) {
    auto overloaded = sharp::overload(
        [](double d) { return d; },
        bar);
    EXPECT_TRUE((std::is_same<decltype(overloaded(1.2)), double>::value));
    EXPECT_EQ(overloaded(1.2), 1.2);
    EXPECT_TRUE((std::is_same<decltype(overloaded(1)), int>::value));
    EXPECT_EQ(overloaded(1), 1);
}

TEST(Overload, BasicInterleavedTest) {
    auto overloaded = sharp::overload(
        [](double d) { return d; },
        bar,
        [](char ch) { return ch; },
        foo);
    EXPECT_TRUE((std::is_same<decltype(overloaded(1.2)), double>::value));
    EXPECT_EQ(overloaded(1.2), 1.2);
    EXPECT_TRUE((std::is_same<decltype(overloaded(1)), int>::value));
    EXPECT_EQ(overloaded(1), 1);
    EXPECT_TRUE((std::is_same<decltype(overloaded('a')), char>::value));
    EXPECT_EQ(overloaded('a'), 'a');
    EXPECT_TRUE((std::is_same<decltype(overloaded()), void>::value));
}

TEST(Overload, TestRefnessWithFunctors) {
    auto overloaded = sharp::overload(
        [](int&) { return 1; },
        [](const int&) { return 2; },
        [](int&&) { return 3; },
        [](const int&&) { return 4; });

    auto integer = 1;
    const auto& integer_const_ref = integer;
    EXPECT_EQ(overloaded(integer), 1);
    EXPECT_EQ(overloaded(integer_const_ref), 2);
    EXPECT_EQ(overloaded(std::move(integer)), 3);
    EXPECT_EQ(overloaded(std::move(integer_const_ref)), 4);
}

TEST(Overload, TestRefnessWithFunctions) {
    auto overloaded = sharp::overload(lvalue, const_lvalue, rvalue,
            const_rvalue);

    auto integer = 1;
    const auto& integer_const_ref = integer;
    EXPECT_EQ(overloaded(integer), 1);
    EXPECT_EQ(overloaded(integer_const_ref), 2);
    EXPECT_EQ(overloaded(std::move(integer)), 3);
    EXPECT_EQ(overloaded(std::move(integer_const_ref)), 4);
}

TEST(Overload, TestRefnessIntertwined) {
    {
        auto overloaded = sharp::overload(
            [](int&) { return 1; },
            const_lvalue,
            rvalue,
            [](const int&&) { return 4; });

        auto integer = 1;
        const auto& integer_const_ref = integer;
        EXPECT_EQ(overloaded(integer), 1);
        EXPECT_EQ(overloaded(integer_const_ref), 2);
        EXPECT_EQ(overloaded(std::move(integer)), 3);
        EXPECT_EQ(overloaded(std::move(integer_const_ref)), 4);
    }

    {
        auto overloaded = sharp::overload(
            lvalue,
            [](const int&) { return 2; },
            [](int&&) { return 3; },
            const_rvalue);

        auto integer = 1;
        const auto& integer_const_ref = integer;
        EXPECT_EQ(overloaded(integer), 1);
        EXPECT_EQ(overloaded(integer_const_ref), 2);
        EXPECT_EQ(overloaded(std::move(integer)), 3);
        EXPECT_EQ(overloaded(std::move(integer_const_ref)), 4);
    }

    {
        auto overloaded = sharp::overload(
            lvalue,
            [](const int&) { return 2; },
            rvalue,
            [](const int&&) { return 4; });

        auto integer = 1;
        const auto& integer_const_ref = integer;
        EXPECT_EQ(overloaded(integer), 1);
        EXPECT_EQ(overloaded(integer_const_ref), 2);
        EXPECT_EQ(overloaded(std::move(integer)), 3);
        EXPECT_EQ(overloaded(std::move(integer_const_ref)), 4);
    }

    {
        auto overloaded = sharp::overload(
            [](int&) { return 1; },
            const_lvalue,
            [](int&&) { return 3; },
            const_rvalue);

        auto integer = 1;
        const auto& integer_const_ref = integer;
        EXPECT_EQ(overloaded(integer), 1);
        EXPECT_EQ(overloaded(integer_const_ref), 2);
        EXPECT_EQ(overloaded(std::move(integer)), 3);
        EXPECT_EQ(overloaded(std::move(integer_const_ref)), 4);
    }
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

// TEST(Overload, TestAmbiguousOverload) {
    // auto integer = 2;
    // auto overloaded = sharp::overload(
        // +[](const int&) { return 0; },
        // +[](int) { return 1; });
        // // [](int) { return 2; });

    // EXPECT_EQ(overloaded(1), 0);
// }

TEST(Overload, TestFallbackLambdaForwarding) {
    auto overloaded = sharp::overload(
        [](auto&&...) { return 1; },
        +[](int&&) { return 0; });

    EXPECT_EQ(overloaded(1), 0);
}

TEST(Overload, TestRecursiveOverloading) {
    auto overloaded_one = sharp::overload(
        [](int) { return 0; }, [](double) { return 1; });

    EXPECT_EQ(overloaded_one(5), 0);
    EXPECT_EQ(overloaded_one(3.2), 1);

    auto overloaded_two = sharp::overload(
        [](char) { return 2; }, overloaded_one);

    EXPECT_EQ(overloaded_two(5), 0);
    EXPECT_EQ(overloaded_two(3.2), 1);
    EXPECT_EQ(overloaded_two('a'), 2);
}

TEST(Overload, TestMutableLambdas) {
    auto i = 1;
    auto overloaded_one = sharp::overload(
        [i](int) mutable { ++i; return 0; },
        [i](double) mutable { ++i; return 1; });

    EXPECT_EQ(overloaded_one(5), 0);
    EXPECT_EQ(overloaded_one(3.2), 1);

    auto overloaded_two = sharp::overload(
        [](char) { return 2; }, overloaded_one);

    EXPECT_EQ(overloaded_two(5), 0);
    EXPECT_EQ(overloaded_two(3.2), 1);
    EXPECT_EQ(overloaded_two('a'), 2);
}

TEST(Overload, TestFunctionOverloadDetector) {
    using namespace sharp::overload_detail;

    auto one = [](double) {};
    auto two = +[](int&&) {};
    auto three = [](char) {};
    auto four = +[](const int&) {};

    using DetectorOne = FunctionOverloadDetector<0,
          decltype(one), decltype(two), decltype(three), decltype(four)>;
    using DetectorTwo = FunctionOverloadDetector<0,
          decltype(two), decltype(three), decltype(one), decltype(four)>;

    auto integer = 1;
    EXPECT_TRUE((std::is_same<decltype(std::declval<DetectorOne>()(1)),
                              InaccessibleConstant<0>>::value));
    EXPECT_TRUE((std::is_same<
                decltype(std::declval<DetectorTwo>()(integer)),
                InaccessibleConstant<1>>::value));
}

TEST(Overload, TestOverloadFunctionImpl) {
    using namespace sharp::overload_detail;

    auto one = +[]() { return 1; };
    auto two = +[](int&&) { return 0; };

    using Detector = FunctionOverloadDetector<0,
          decltype(one),  decltype(two)>;

    EXPECT_TRUE((std::is_same<decltype(std::declval<Detector>()(1)),
                              InaccessibleConstant<1>>::value));
    EXPECT_TRUE((std::is_same<
                decltype(std::declval<Detector>()()),
                InaccessibleConstant<0>>::value));

    auto overload_impl_function = OverloadImpl<Detector, 0,
         decltype(one),  decltype(two)>{one, two};
    EXPECT_EQ(overload_impl_function(), 1);
    EXPECT_EQ(overload_impl_function(1), 0);
}
