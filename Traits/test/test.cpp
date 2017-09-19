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
#include <sharp/Traits/Traits.hpp>

#include <gtest/gtest.h>

#include <vector>
#include <typeindex>
#include <tuple>
#include <algorithm>
#include <utility>

using namespace sharp;

namespace {
    template <typename ValueListOne, typename ValueListTwo>
    struct LessThanValueList;
    template <int value_one, int value_two>
    struct LessThanValueList<ValueList<value_one>, ValueList<value_two>> {
        static constexpr const bool value = value_one < value_two;
    };
    template <typename One, typename Two>
    struct LessThanSize {
        static constexpr const bool value = sizeof(One) < sizeof(Two);
    };

    class MemberFunctions {
    public:
        void a() {}
        void b() const{}
        void c() & {}
        void d() const & {}
        void e() && {}
        void f() const && {}
    };

    class Functor {
    public:
        int operator()(int, double) { return int{}; }
    };
    double some_function(int, char) { return double{}; }

    class TemplatedFunctor {
    public:
        template <typename T>
        void operator()(T) {}
    };

    class OverloadedFunctor {
    public:
        void operator()(int) {}
        void operator()(char) {}
    };

} // namespace <anonymous>

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

TEST(Traits, ForEach) {
    auto vec = std::vector<std::type_index>{typeid(int), typeid(double)};
    auto result_vec = decltype(vec){};

    ForEach<std::tuple<int, double>>{}([&](auto type_context) {
        result_vec.push_back(typeid(typename decltype(type_context)::type));
    });

    EXPECT_TRUE(std::equal(vec.begin(), vec.end(), result_vec.begin()));
}

TEST(Traits, AllOf) {
    EXPECT_TRUE((AllOf_v<std::is_reference, std::tuple<>>));
    EXPECT_TRUE((AllOf_v<std::is_reference, std::tuple<int&>>));
    EXPECT_FALSE((AllOf_v<std::is_reference, std::tuple<int&, double>>));
    EXPECT_FALSE((AllOf_v<std::is_reference, std::tuple<int, double&>>));
    EXPECT_TRUE((AllOf_v<std::is_reference, std::tuple<int&, double&>>));
}

TEST(Traits, AnyOf) {
    EXPECT_FALSE((AnyOf_v<std::is_reference, std::tuple<>>));
    EXPECT_TRUE((AnyOf_v<std::is_reference, std::tuple<int&>>));
    EXPECT_TRUE((AnyOf_v<std::is_reference, std::tuple<int&, double>>));
    EXPECT_TRUE((AnyOf_v<std::is_reference, std::tuple<int, double&>>));
    EXPECT_FALSE((AnyOf_v<std::is_reference, std::tuple<int*, double*>>));
    EXPECT_TRUE((AnyOf_v<std::is_reference, std::tuple<int&, double&>>));
}

TEST(Traits, NoneOf) {
    EXPECT_TRUE((NoneOf_v<std::is_reference, std::tuple<>>));
    EXPECT_FALSE((NoneOf_v<std::is_reference, std::tuple<int&>>));
    EXPECT_FALSE((NoneOf_v<std::is_reference, std::tuple<int&, double>>));
    EXPECT_FALSE((NoneOf_v<std::is_reference, std::tuple<int, double&>>));
    EXPECT_TRUE((NoneOf_v<std::is_reference, std::tuple<int*, double*>>));
    EXPECT_FALSE((NoneOf_v<std::is_reference, std::tuple<int&, double&>>));
}

TEST(Traits, CountIf) {
    EXPECT_TRUE((CountIf_v<std::is_reference, std::tuple<>> == 0));
    EXPECT_TRUE((CountIf_v<std::is_reference, std::tuple<int&>> == 1));
    EXPECT_TRUE((CountIf_v<std::is_reference, std::tuple<int&, double>> == 1));
    EXPECT_TRUE((CountIf_v<std::is_reference, std::tuple<int&, double&>> == 2));
}

TEST(Traits, MaxValue) {
    EXPECT_TRUE((MaxValue_v<> == -1));
    EXPECT_TRUE((MaxValue_v<1> == 1));
    EXPECT_TRUE((MaxValue_v<1, 2> == 2));
    EXPECT_TRUE((MaxValue_v<1, 2, 3> == 3));
    EXPECT_TRUE((MaxValue_v<-1, 2, 3> == 3));
}

TEST(Traits, MaxType) {
    EXPECT_TRUE((std::is_same<MaxType_t<LessThanValueList,
                                        ValueList<0>, ValueList<1>>,
                              ValueList<1>>::value));
    EXPECT_TRUE((std::is_same<MaxType_t<LessThanValueList,
                                        ValueList<1>, ValueList<0>>,
                              ValueList<1>>::value));
}

TEST(Traits, MinValue) {
    EXPECT_TRUE((MinValue_v<> == -1));
    EXPECT_TRUE((MinValue_v<1> == 1));
    EXPECT_TRUE((MinValue_v<1, 2> == 1));
    EXPECT_TRUE((MinValue_v<1, 2, 3> == 1));
    EXPECT_TRUE((MinValue_v<-1, 2, 3> == -1));
}

TEST(Traits, MinType) {
    EXPECT_TRUE((std::is_same<MinType_t<LessThanValueList,
                                        ValueList<0>, ValueList<1>>,
                              ValueList<0>>::value));
    EXPECT_TRUE((std::is_same<MinType_t<LessThanValueList,
                                        ValueList<1>, ValueList<0>>,
                              ValueList<0>>::value));
}

TEST(Traits, Mismatch) {
    EXPECT_TRUE((std::is_same<Mismatch_t<std::tuple<int, double, char>,
                                         std::tuple<int, double, char*>>,
                              std::pair<std::tuple<char>, std::tuple<char*>>>
                              ::value));
    EXPECT_TRUE((std::is_same<Mismatch_t<std::tuple<>, std::tuple<>>,
                              std::pair<std::tuple<>, std::tuple<>>>::value));
    EXPECT_TRUE((std::is_same<Mismatch_t<std::tuple<int, double>, std::tuple<>>,
                              std::pair<std::tuple<int, double>, std::tuple<>>>
                              ::value));
    EXPECT_TRUE((std::is_same<Mismatch_t<std::tuple<>, std::tuple<int, double>>,
                              std::pair<std::tuple<>, std::tuple<int, double>>>
                              ::value));
    EXPECT_TRUE((std::is_same<Mismatch_t<std::tuple<int, char*>,
                                         std::tuple<int, double>>,
                              std::pair<std::tuple<char*>, std::tuple<double>>>
                              ::value));
    EXPECT_TRUE((std::is_same<Mismatch_t<std::tuple<int, char*>,
                                         std::tuple<int, double, bool, char>>,
                              std::pair<std::tuple<char*>,
                                        std::tuple<double, bool, char>>>
                                        ::value));
}

TEST(Traits, Equal) {
    EXPECT_TRUE((Equal_v<std::tuple<int, double>, std::tuple<int, double>>));
    EXPECT_TRUE((Equal_v<std::tuple<>, std::tuple<>>));
    EXPECT_TRUE((Equal_v<std::tuple<int, double>,
                         std::tuple<int, double, int>>));
    EXPECT_FALSE((Equal_v<std::tuple<int, double, char>,
                          std::tuple<int, double>>));
    EXPECT_FALSE((Equal_v<std::tuple<double, char>, std::tuple<int, double>>));
    EXPECT_FALSE((Equal_v<std::tuple<int, double, char>, std::tuple<double>>));
}

TEST(Traits, FindIf) {
    EXPECT_TRUE((std::is_same<FindIf_t<std::is_reference, std::tuple<>>,
                             std::tuple<>>::value));
    EXPECT_TRUE((std::is_same<FindIf_t<std::is_reference,
                                       std::tuple<int, int&>>,
                                       std::tuple<int&>>::value));
    EXPECT_TRUE((std::is_same<FindIf_t<std::is_reference,
                                       std::tuple<int*, int&>>,
                                       std::tuple<int&>>::value));
    EXPECT_TRUE((std::is_same<FindIf_t<std::is_reference,
                                       std::tuple<double, int>>,
                                       std::tuple<>>::value));
    EXPECT_TRUE((std::is_same<FindIf_t<std::is_reference,
                                       std::tuple<double&, int>>,
                                       std::tuple<double&, int>>::value));
}

TEST(Traits, Find) {
    EXPECT_TRUE((std::is_same<Find_t<int, std::tuple<>>, std::tuple<>>::value));
    EXPECT_TRUE((std::is_same<Find_t<int, std::tuple<double, int>>,
                              std::tuple<int>>::value));
    EXPECT_TRUE((std::is_same<Find_t<int, std::tuple<int, double>>,
                              std::tuple<int, double>>::value));
    EXPECT_TRUE((std::is_same<Find_t<int, std::tuple<double*, int>>,
                              std::tuple<int>>::value));
    EXPECT_TRUE((std::is_same<Find_t<int, std::tuple<double*, int, bool>>,
                              std::tuple<int, bool>>::value));
}

TEST(Traits, FindIndex) {
    EXPECT_TRUE((FindIndex_v<int, std::tuple<>> == 0));
    EXPECT_TRUE((FindIndex_v<int, std::tuple<double, int>> == 1));
    EXPECT_TRUE((FindIndex_v<int, std::tuple<int, double>> == 0));
    EXPECT_TRUE((FindIndex_v<int, std::tuple<double*, int>> == 1));
    EXPECT_TRUE((FindIndex_v<int, std::tuple<double*, int, bool>> == 1));
}

TEST(Traits, FindIfNot) {
    EXPECT_TRUE((std::is_same<FindIfNot_t<std::is_reference, std::tuple<>>,
                              std::tuple<>>::value));
    EXPECT_TRUE((std::is_same<FindIfNot_t<std::is_reference,
                                          std::tuple<int, int&>>,
                              std::tuple<int, int&>>::value));
    EXPECT_TRUE((std::is_same<FindIfNot_t<std::is_reference,
                                          std::tuple<int*, int&>>,
                              std::tuple<int*, int&>>::value));
    EXPECT_TRUE((std::is_same<FindIfNot_t<std::is_reference,
                                          std::tuple<int&, double, int>>,
                              std::tuple<double, int>>::value));
}

TEST(Traits, FindFirstOf) {
    EXPECT_TRUE((std::is_same<FindFirstOf_t<std::tuple<>, std::tuple<>>,
                              std::tuple<>>::value));
    EXPECT_TRUE((std::is_same<FindFirstOf_t<std::tuple<int>, std::tuple<>>,
                              std::tuple<>>::value));
    EXPECT_TRUE((std::is_same<FindFirstOf_t<std::tuple<>, std::tuple<int>>,
                              std::tuple<>>::value));
    EXPECT_TRUE((std::is_same<FindFirstOf_t<std::tuple<int, double>,
                                            std::tuple<>>,
                              std::tuple<>>::value));
    EXPECT_TRUE((std::is_same<FindFirstOf_t<std::tuple<>,
                                             std::tuple<int, double>>,
                              std::tuple<>>::value));
    EXPECT_TRUE((std::is_same<FindFirstOf_t<std::tuple<int, double>,
                                            std::tuple<char, double>>,
                              std::tuple<double>>::value));
    EXPECT_TRUE((std::is_same<FindFirstOf_t<std::tuple<int, double*>,
                                            std::tuple<char, double>>,
                              std::tuple<>>::value));
    EXPECT_TRUE((std::is_same<FindFirstOf_t<std::tuple<int, double*>,
                                            std::tuple<int, double>>,
                              std::tuple<int, double*>>::value));
}

TEST(Traits, AdjacentFind) {
    EXPECT_TRUE((std::is_same<AdjacentFind_t<std::tuple<int, double, char>>,
                              std::tuple<>>::value));
    EXPECT_TRUE((std::is_same<AdjacentFind_t<std::tuple<int, int, char>>,
                              std::tuple<int, int, char>>::value));
    EXPECT_TRUE((std::is_same<AdjacentFind_t<std::tuple<char, int, int>>,
                              std::tuple<int, int>>::value));
    EXPECT_TRUE((std::is_same<AdjacentFind_t<std::tuple<int*, double&, int*>>,
                              std::tuple<>>::value));
}

TEST(Traits, Search) {
    EXPECT_TRUE((std::is_same<Search_t<std::tuple<double, char>,
                                       std::tuple<double, char>>,
                              std::tuple<double, char>>::value));
    EXPECT_TRUE((std::is_same<Search_t<std::tuple<int, double, char>,
                                       std::tuple<double, char>>,
                              std::tuple<double, char>>::value));
    EXPECT_TRUE((std::is_same<Search_t<std::tuple<int, double, int, char>,
                                       std::tuple<double, int>>,
                              std::tuple<double, int, char>>::value));
    EXPECT_TRUE((std::is_same<Search_t<std::tuple<double>,
                                       std::tuple<double, int>>,
                              std::tuple<>>::value));
    EXPECT_TRUE((std::is_same<Search_t<std::tuple<int, double, char>,
                                       std::tuple<int>>,
                              std::tuple<int, double, char>>::value));
    EXPECT_TRUE((std::is_same<Search_t<std::tuple<int, double, char>,
                                       std::tuple<double>>,
                              std::tuple<double, char>>::value));
    EXPECT_TRUE((std::is_same<Search_t<std::tuple<int, double, char>,
                                       std::tuple<>>,
                              std::tuple<int, double, char>>::value));
    EXPECT_TRUE((std::is_same<Search_t<std::tuple<int, double, char, float>,
                                       std::tuple<double, char>>,
                              std::tuple<double, char, float>>::value));
    EXPECT_TRUE((std::is_same<Search_t<std::tuple<>,
                                       std::tuple<>>,
                              std::tuple<>>::value));
    EXPECT_TRUE((std::is_same<Search_t<
                                std::tuple<double, int, int, int, double>,
                                std::tuple<int, int, int>>,
                              std::tuple<int, int, int, double>>::value));
}

TEST(Traits, SearchN) {
    EXPECT_TRUE((std::is_same<SearchN_t<
                                int, 3,
                                std::tuple<double, int, int, int, double>>,
                              std::tuple<int, int, int, double>>::value));
    EXPECT_TRUE((std::is_same<SearchN_t<
                                int, 1,
                                std::tuple<double, int, int, int, double>>,
                              std::tuple<int, int, int, double>>::value));
    EXPECT_TRUE((std::is_same<SearchN_t<
                                double, 2,
                                std::tuple<double, int, double, double, int>>,
                              std::tuple<double, double, int>>::value));
    EXPECT_TRUE((std::is_same<SearchN_t<
                                double, 3,
                                std::tuple<double, int, int, int, double>>,
                              std::tuple<>>::value));
}

TEST(Traits, TransformIf) {
    EXPECT_TRUE((std::is_same<TransformIf_t<std::is_reference,
                                           std::remove_reference,
                                           std::tuple<int, double, int&, char>>,
                             std::tuple<int, double, int, char>>::value));
    EXPECT_TRUE((std::is_same<TransformIf_t<
                                std::is_reference,
                                std::remove_reference,
                                std::tuple<int*, double&, int&, char>>,
                              std::tuple<int*, double, int, char>>::value));
    EXPECT_TRUE((std::is_same<TransformIf_t<std::is_reference,
                                            std::remove_reference,
                                            std::tuple<int>>,
                              std::tuple<int>>::value));
}

TEST(Traits, Transform) {
    EXPECT_TRUE((std::is_same<Transform_t<std::remove_reference,
                                          std::tuple<int&, double&>>,
                              std::tuple<int, double>>::value));
    EXPECT_TRUE((std::is_same<
            Transform_t<std::remove_pointer,
                        std::tuple<std::add_pointer_t<int&>, double&>>,
            std::tuple<int, double&>>::value));
    EXPECT_TRUE((std::is_same<Transform_t<std::remove_reference, std::tuple<>>,
                              std::tuple<>>::value));
    EXPECT_TRUE((std::is_same<Transform_t<
                                std::decay,
                                std::tuple<const int&, volatile char&>>,
                              std::tuple<int, char>>::value));
}

TEST(Traits, RemoveIf) {
    EXPECT_TRUE((std::is_same<RemoveIf_t<std::is_reference,
                                         std::tuple<int, int&, char>>,
                             std::tuple<int, char>>::value));
    EXPECT_TRUE((std::is_same<RemoveIf_t<std::is_pointer,
                                         std::tuple<int*, int*, char>>,
                              std::tuple<char>>::value));
    EXPECT_TRUE((std::is_same<RemoveIf_t<std::is_pointer,
                                         std::tuple<int, int, char>>,
                              std::tuple<int, int, char>>::value));
    EXPECT_TRUE((std::is_same<RemoveIf_t<std::is_pointer, std::tuple<int*>>,
                              std::tuple<>>::value));
    EXPECT_TRUE((std::is_same<RemoveIf_t<std::is_pointer, std::tuple<int>>,
                              std::tuple<int>>::value));
    EXPECT_TRUE((std::is_same<RemoveIf_t<std::is_pointer, std::tuple<>>,
                              std::tuple<>>::value));
}

TEST(Traits, Reverse) {
    EXPECT_TRUE((std::is_same<Reverse_t<std::tuple<int, char>>,
                              std::tuple<char, int>>::value));
    EXPECT_TRUE((std::is_same<Reverse_t<std::tuple<>>, std::tuple<>>::value));
    EXPECT_TRUE((std::is_same<Reverse_t<std::tuple<int, char, bool, double>>,
                              std::tuple<double, bool, char, int>>::value));
    EXPECT_TRUE((std::is_same<Reverse_t<std::tuple<int>>,
                              std::tuple<int>>::value));
    EXPECT_TRUE((std::is_same<Reverse_t<std::tuple<char, int>>,
                              std::tuple<int, char>>::value));
}

TEST(Traits, Unique) {
    EXPECT_TRUE((std::is_same<Unique_t<std::tuple<int, double, int>>,
                              std::tuple<int, double>>::value));
    EXPECT_TRUE((std::is_same<Unique_t<std::tuple<int, int, double>>,
                              std::tuple<int, double>>::value));
    EXPECT_TRUE((std::is_same<Unique_t<std::tuple<double, int, int>>,
                              std::tuple<double, int>>::value));
}

TEST(Traits, Sort) {
    EXPECT_TRUE((std::is_same<Sort_t<LessThanSize,
                                     std::tuple<std::uint32_t, std::uint16_t,
                                                std::uint8_t>>,
                              std::tuple<std::uint8_t, std::uint16_t,
                                         std::uint32_t>>::value));
    EXPECT_TRUE((std::is_same<Sort_t<LessThanSize,
                                     std::tuple<std::uint8_t, std::uint16_t,
                                                std::uint32_t>>,
                              std::tuple<std::uint8_t, std::uint16_t,
                                         std::uint32_t>>::value));
    EXPECT_TRUE((std::is_same<Sort_t<LessThanSize,
                                     std::tuple<std::uint16_t, std::uint8_t,
                                                std::uint32_t>>,
                              std::tuple<std::uint8_t, std::uint16_t,
                                         std::uint32_t>>::value));
    EXPECT_TRUE((std::is_same<Sort_t<LessThanSize,
                                     std::tuple<std::uint16_t, std::uint8_t,
                                                std::uint32_t>>,
                              std::tuple<std::uint8_t, std::uint16_t,
                                         std::uint32_t>>::value));
    EXPECT_TRUE((std::is_same<Sort_t<LessThanSize,
                                     std::tuple<std::uint16_t, std::uint32_t,
                                                std::uint8_t>>,
                              std::tuple<std::uint8_t, std::uint16_t,
                                         std::uint32_t>>::value));
    EXPECT_TRUE((std::is_same<Sort_t<LessThanSize,
                                     std::tuple<std::uint8_t, std::uint32_t,
                                                std::uint16_t>>,
                              std::tuple<std::uint8_t, std::uint16_t,
                                         std::uint32_t>>::value));
}

TEST(Traits, Concatenate_t) {
    EXPECT_TRUE((std::is_same<Concatenate_t<
                std::tuple<int>, std::tuple<double>>,
                std::tuple<int, double>>::value));
    EXPECT_TRUE((std::is_same<Concatenate_t<ValueList<0>, ValueList<1>>,
                                            ValueList<0, 1>>::value));
    EXPECT_TRUE((std::is_same<Concatenate_t<std::tuple<char>, std::tuple<int>,
                                            std::tuple<bool>, std::tuple<int>>,
                              std::tuple<char, int, bool, int>>::value));
    EXPECT_TRUE((std::is_same<Concatenate_t<ValueList<0>, ValueList<1>,
                                            ValueList<2>, ValueList<3>>,
                              ValueList<0, 1, 2, 3>>::value));
}

TEST(Traits, PopFront_t) {
    EXPECT_TRUE((std::is_same<PopFront_t<std::tuple<int, double>>,
                              std::tuple<double>>::value));
    EXPECT_TRUE((std::is_same<PopFront_t<std::tuple<double>>,
                              std::tuple<>>::value));
    EXPECT_TRUE((std::is_same<PopFront_t<std::tuple<>>,
                              std::tuple<>>::value));
}

TEST(Traits, Erase_t) {
    EXPECT_TRUE((std::is_same<Erase_t<0, std::tuple<int, double, char>>,
                              std::tuple<double, char>>::value));
    EXPECT_TRUE((std::is_same<Erase_t<1, std::tuple<int, double, char>>,
                               std::tuple<int, char>>::value));
    EXPECT_TRUE((std::is_same<Erase_t<2, std::tuple<int, double, char>>,
                               std::tuple<int, double>>::value));
    EXPECT_TRUE((std::is_same<Erase_t<0, std::tuple<>>, std::tuple<>>::value));
    EXPECT_TRUE((std::is_same<Erase_t<1, std::tuple<>>, std::tuple<>>::value));
}

TEST(Traits, ConcatenateN_t) {
    EXPECT_TRUE((std::is_same<ConcatenateN_t<int, 3>,
                              std::tuple<int, int, int>>::value));
    EXPECT_TRUE((std::is_same<ConcatenateN_t<int, 0>,
                              std::tuple<>>::value));
    EXPECT_TRUE((std::is_same<ConcatenateN_t<int, 1>,
                              std::tuple<int>>::value));
}

TEST(Traits, MatchReference_t) {
    EXPECT_TRUE((std::is_same<MatchReference_t<int&, double>, double&>::value));
    EXPECT_TRUE((std::is_same<MatchReference_t<const int&, double>,
                              const double&>::value));
    EXPECT_TRUE((std::is_same<MatchReference_t<int&&, double>, double&&>
                ::value));
    EXPECT_TRUE((std::is_same<MatchReference_t<const int&&, double>,
                              const double&&>::value));
}

TEST(Traits, ReturnType_t) {
    static_cast<void>(some_function);
    EXPECT_TRUE((std::is_same<sharp::ReturnType_t<Functor>, int>::value));
    EXPECT_TRUE((std::is_same<sharp::ReturnType_t<
            decltype(some_function)>, double>::value));
    EXPECT_TRUE((std::is_same<sharp::ReturnType_t<
            decltype(&some_function)>, double>::value));

    // The following static_assert should give an error when uncommented because
    // int is not callable
    // EXPECT_TRUE((std::is_same<sharp::ReturnType_t<int>, double>::value));
}

TEST(Traits, Arguments_t) {
    EXPECT_TRUE((std::is_same<Arguments_t<Functor>,
                               sharp::Args<int, double>>::value));
    EXPECT_TRUE((std::is_same<Arguments_t<decltype(some_function)>,
                              sharp::Args<int, char>>::value));
    EXPECT_TRUE((std::is_same<Arguments_t<decltype(&some_function)>,
                              sharp::Args<int, char>>::value));
    EXPECT_TRUE((std::is_same<Arguments_t<TemplatedFunctor>,
                              sharp::Args<sharp::Unspecified>>::value));
    EXPECT_TRUE((std::is_same<Arguments_t<OverloadedFunctor>,
                              sharp::Args<sharp::Unspecified>>::value));
}

TEST(Traits, WhichInvocableType) {
    auto functor = []{};
    using Functor = decltype(functor);
    auto function_pointer = +[]{};
    using FunctionPointer = decltype(function_pointer);

    using A = decltype(&MemberFunctions::a);
    using B = decltype(&MemberFunctions::b);
    using C = decltype(&MemberFunctions::c);
    using D = decltype(&MemberFunctions::d);
    using E = decltype(&MemberFunctions::e);
    using F = decltype(&MemberFunctions::f);

    EXPECT_EQ((sharp::WhichInvocableType_v<FunctionPointer>),
              sharp::F_PTR);
    EXPECT_EQ((sharp::WhichInvocableType_v<Functor>),
              sharp::FUNCTOR);
    EXPECT_EQ((sharp::WhichInvocableType_v<A>),
              sharp::MEMBER_F_PTR);
    EXPECT_EQ((sharp::WhichInvocableType_v<B>),
              sharp::MEMBER_F_PTR);
    EXPECT_EQ((sharp::WhichInvocableType_v<C>),
              sharp::MEMBER_F_PTR);
    EXPECT_EQ((sharp::WhichInvocableType_v<D>),
              sharp::MEMBER_F_PTR);
    EXPECT_EQ((sharp::WhichInvocableType_v<E>),
              sharp::MEMBER_F_PTR);
    EXPECT_EQ((sharp::WhichInvocableType_v<F>),
              sharp::MEMBER_F_PTR);
}
