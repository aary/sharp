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
    double bar(int) { return 1.0; }

    template <typename...> struct WhichType;

} // namespace <anonymous>

TEST(Overload, BasicOverloadTest) {
    auto overloaded = sharp::make_overload(
        [](int a) { return a; },
        [](double d) { return d; });
        // foo);

    EXPECT_EQ(overloaded(1), 1);
    EXPECT_EQ(overloaded(2.1), 2.1);
    // EXPECT_TRUE((std::is_same<decltype(overloaded()), void>::value));
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
                Two&&, void (&) (), double (&) (int)>>::value));
}
