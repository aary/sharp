#include <sharp/Overload/Overload.hpp>
#include <sharp/Utility/Utility.hpp>

#include <gtest/gtest.h>

#include <cstdint>

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

    class MemberFunctions {
    public:
        int lvalue(int&) { return 1; }
        int const_lvalue(const int&) { return 2; }
        int rvalue(int&&) { return 3; }
        int const_rvalue(const int&&) { return 4; }
    };

    /**
     * Do different things with const overloads
     */
    class ConstOverloadedFunctor {
    public:
        int operator()() {
            return 0;
        }
        int operator()() const {
            return 1;
        }
    };

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

TEST(Overload, BasicMemberFunctionOverloadTest) {
    auto overloaded = sharp::overload(
        &MemberFunctions::lvalue, &MemberFunctions::rvalue,
        &MemberFunctions::const_lvalue, &MemberFunctions::const_rvalue );

    auto instance = MemberFunctions{};
    auto integer = 1;
    const auto const_integer = 2;
    EXPECT_EQ(overloaded(instance, integer), 1);
    EXPECT_EQ(overloaded(instance, const_integer), 2);
    EXPECT_EQ(overloaded(instance, 1), 3);
    EXPECT_EQ(overloaded(instance, std::move(const_integer)), 4);
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

TEST(Overload, MemberFunctionFunctionPointerTest) {
    auto overloaded = sharp::overload(
        &MemberFunctions::lvalue, rvalue);

    auto integer = 1;
    EXPECT_EQ(overloaded(MemberFunctions{}, integer), 1);
    EXPECT_EQ(overloaded(1), 3);
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
            typename overload_detail::SplitLists<
                ValueList<>, ValueList<>, ValueList<>,
                0,
                sharp::ValueList<FUNCTOR, F_PTR, FUNCTOR, F_PTR>>::type,
            std::tuple<ValueList<0, 2>, ValueList<1, 3>, ValueList<>>>::value));
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
        [](auto&&...) { return 2; },
        +[](int&&) { return 0; },
        +[](double&&) { return 1; });

    EXPECT_EQ(overloaded(1), 0);
    EXPECT_EQ(overloaded(1.2), 1);
}

TEST(Overload, TestCopyConstructorWorks) {
    auto overloaded = sharp::overload([](int) {}, [](double) {});
    auto copied = overloaded;
    static_cast<void>(copied);
}

TEST(Overload, TestMoveConstructorWorks) {
    auto overloaded = sharp::overload([](int) {}, [](double) {});
    auto moved = std::move(overloaded);
    static_cast<void>(moved);
}

TEST(Overload, TestRecursiveOverloading) {
    auto overloaded_one = sharp::overload(
        +[](int) { return 0; }, [](double) { return 1; });

    EXPECT_EQ(overloaded_one(5), 0);
    EXPECT_EQ(overloaded_one(3.2), 1);

    auto overloaded_two = sharp::overload(
        [](char) { return 2; }, overloaded_one);

    EXPECT_EQ(overloaded_two(5), 0);
    EXPECT_EQ(overloaded_two(3.2), 1);
    EXPECT_EQ(overloaded_two('a'), 2);

    EXPECT_TRUE((std::is_same<
                decltype(overloaded_two)::ArgumentsList,
                std::tuple<sharp::Args<char>, sharp::Args<int>,
                           sharp::Args<double>>>::value));
}

TEST(Overload, TestMutableLambdas) {
    auto i = 1;
    auto overloaded_one = sharp::overload(
        [i](int) mutable { ++i; return 0; },
        [i](double) mutable { ++i; return 1; },
        []() {});

    EXPECT_EQ(overloaded_one(5), 0);
    EXPECT_EQ(overloaded_one(3.2), 1);

    const auto overloaded_two = sharp::overload(
        [](char) { return 2; }, std::move(overloaded_one));

    // TODO
    // This does not throw a warning, idk why, should throw a literal
    // conversion warning
    EXPECT_EQ(overloaded_two(5), 2);
    EXPECT_EQ(overloaded_two(3.2), 2);
    EXPECT_EQ(overloaded_two('a'), 2);
}

TEST(Overload, ConstOverloadFunctor) {
    auto overloaded = sharp::overload(ConstOverloadedFunctor{});
    const auto& overloaded_const = overloaded;
    EXPECT_EQ(overloaded(), 0);
    EXPECT_EQ(overloaded_const(), 1);
}

TEST(Overload, TestFlattener) {
    using namespace sharp::overload_detail;
    {
        auto args = flatten_args(std::make_index_sequence<2>{},
                std::forward_as_tuple(foo, bar));
        EXPECT_TRUE((std::is_same<decltype(args),
                                  std::tuple<void (&) (), int (&) (int)>
            >::value));
    }
    {
        auto one = +[] {};
        auto two = +[](int) {};
        auto args = flatten_args(std::make_index_sequence<2>{},
                std::forward_as_tuple(std::move(one), std::move(two)));
        EXPECT_TRUE((std::is_same<decltype(args),
                                   std::tuple<void (*&&) (), void (*&&) (int)>
            >::value));
    }
    {
        auto one = [] {};
        auto two = [](int) {};
        auto args = flatten_args(std::make_index_sequence<2>{},
                std::forward_as_tuple(std::move(one), two));
        EXPECT_TRUE((std::is_same<decltype(args),
                                   std::tuple<decltype(one)&&, decltype(two)&>
            >::value));
    }
    {
        auto one = [] {};
        auto two = +[](int) {};
        auto args = flatten_args(std::make_index_sequence<2>{},
                std::forward_as_tuple(std::move(one), two));
        EXPECT_TRUE((std::is_same<decltype(args),
                                   std::tuple<decltype(one)&&, void (*&) (int)>
            >::value));
    }
}

TEST(Overload, TestFunctionOverloadDetector) {
    using namespace sharp::overload_detail;

    auto one = [](double) {};
    auto two = +[](int&&) {};
    auto three = [](char) {};
    auto four = +[](const int&) {};
    auto five = [](auto&&...) {};

    {
        using Detector = FunctionOverloadDetector<0, 0,
              decltype(one), decltype(two), decltype(three), decltype(four),
              decltype(five)>;

        EXPECT_TRUE((std::is_same<decltype(std::declval<Detector>()(1)),
                                  FPtrConstant<0>>::value));
    }
    {
        const auto& lvalue = 1;
        using Detector = FunctionOverloadDetector<0, 0,
              decltype(two), decltype(three), decltype(one), decltype(four),
              decltype(five)>;
        EXPECT_TRUE((std::is_same<decltype(std::declval<Detector>()(lvalue)),
                                  FPtrConstant<1>>::value));
    }

    {
        auto one = [](int) mutable { return int{}; };
        auto two = [](double) mutable { return double{}; };
        auto three = [](char) { return char{}; };
        using Detector = const FunctionOverloadDetector<0, 0,
              decltype(one), decltype(two), decltype(three)>;

        EXPECT_TRUE((std::is_same<decltype(std::declval<Detector>()('a')), char>
                    ::value));
    }

    {
        auto one = &MemberFunctions::lvalue;
        auto two = &MemberFunctions::const_lvalue;
        auto three = &MemberFunctions::rvalue;
        auto four = &MemberFunctions::const_rvalue;

        using Detector = FunctionOverloadDetector<0, 0,
              decltype(one), decltype(two), decltype(three), decltype(four)>;

        auto i = 1;
        auto instance = MemberFunctions{};
        EXPECT_TRUE((std::is_same<
                    decltype(std::declval<Detector>()(
                            instance,
                            i)),
                    MemberFPtrConstant<0>>::value));
        EXPECT_TRUE((std::is_same<
                    decltype(std::declval<Detector>()(
                            instance,
                            sharp::as_const(i))),
                    MemberFPtrConstant<1>>::value));
        EXPECT_TRUE((std::is_same<
                    decltype(std::declval<Detector>()(
                            instance,
                            sharp::decay_copy(i))),
                    MemberFPtrConstant<2>>::value));
        EXPECT_TRUE((std::is_same<
                    decltype(std::declval<Detector>()(
                            instance,
                            static_cast<const int&&>(i))),
                    MemberFPtrConstant<3>>::value));
    }
}

TEST(Overload, TestFunctionOverloadDetectorFallback) {
    using namespace sharp::overload_detail;
    auto one = +[](int) {};
    auto two = [](auto&&...) {};

    using Detector = FunctionOverloadDetector<0, 0, decltype(one),
                                              decltype(two)>;
    EXPECT_TRUE((!std::is_same<decltype(std::declval<Detector>()(1.2)),
                               FPtrConstant<0>>::value));
}
