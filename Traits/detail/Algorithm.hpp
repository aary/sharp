/**
 * @file TypeAlgorithm.hpp
 * @author Aaryaman Sagar
 *
 * This file contains algorithms that can be executed on a template pack,
 * including things similar to std::any_of std::all_of std::max and so on
 */

#pragma once

#include <type_traits>
#include <tuple>

#include <sharp/Traits/detail/Functional.hpp>
#include <sharp/Traits/detail/Utility.hpp>

namespace sharp {

/**
 * @class End
 *
 * A tag that denotes the end of a type list range, similar to std::end() this
 * marks the end of a type list range.  This is used in cases where an
 * algorithm returns past the end of a range to denote that a value could not
 * be found.
 *
 * For example if the predicate passed to FindIf returns true for none of the
 * types then the algorithm returns an End tag to denote failure
 */
struct End {};

namespace detail {

    /**
     * Implementation for the AllOf trait
     */
    template <template <typename...> class Predicate, typename... Types>
    struct AllOfImpl {
        static constexpr const bool value = true;
    };
    template <template <typename...> class Predicate,
              typename Head, typename... Tail>
    struct AllOfImpl<Predicate, Head, Tail...> {
        static constexpr const bool value
            = Predicate<Head>::value
                && detail::AllOfImpl<Predicate, Tail...>::value;
    };

    /**
     * Implementation for the AnyOf trait, this unlike the AllOf trait returns
     * a false when the list is empty, in line with std::any_of
     */
    template <template <typename...> class Predicate, typename... Types>
    struct AnyOfImpl {
        static constexpr const bool value = false;
    };
    template <template <typename...> class Predicate,
              typename Head, typename... Tail>
    struct AnyOfImpl<Predicate, Head, Tail...> {
        static constexpr const bool value
            = Predicate<Head>::value
                || detail::AnyOfImpl<Predicate, Tail...>::value;
    };

    /**
     * Implementation for the count if trait
     */
    template <template <typename...> class Predicate, typename... Types>
    struct CountIfImpl {
        static constexpr const int value = 0;
    };
    template <template <typename...> class Predicate,
              typename Head, typename... Tail>
    struct CountIfImpl<Predicate, Head, Tail...> {
        /**
         * convert to an int and add recursively
         */
        static constexpr const int value
            = int(Predicate<Head>::value)
                + CountIfImpl<Predicate, Tail...>::value;
    };

    /**
     * Implementation for the FindIf trait
     */
    template <template <typename...> class Predicate, typename... Types>
    struct FindIfImpl {
        using type = End;
    };
    template <template <typename...> class Predicate,
              typename Head, typename... Tail>
    struct FindIfImpl<Predicate, Head, Tail...> {
        using type = std::conditional_t<
            Predicate<Head>::value,
            std::tuple<Head, Tail...>,
            typename FindIfImpl<Predicate, Tail...>::type>;
    };

    /**
     * Implementation for the FindFirstOfTrait
     */
    template <typename TypesContainerOne, typename TypesContainerTwo>
    struct FindFirstOfImpl;
    template <typename HeadOne, typename... TailOne,
              typename... TailTwo>
    struct FindFirstOfImpl<std::tuple<HeadOne, TailOne...>,
                           std::tuple<TailTwo...>> {

        /**
         * Iterate through the second list using the first head as what you
         * check for equality, if that returns a false, i.e. if the first
         * element was not in the second list then do the same for the
         * expanded pack on the first list
         *
         * So if the lists are <int, double> and <char, bool> check int to see
         * if int exists in <char, bool>, if it doesnt then check to see if
         * double exists in <char, bool> and so on
         */
        using type = std::conditional_t<!std::is_same<
            typename FindIfImpl<
                       Bind<std::is_same, HeadOne>::template type,
                       TailTwo...>::type,
            End>::value,
            std::tuple<HeadOne, TailOne...>,
            typename FindFirstOfImpl<std::tuple<TailOne...>,
                                     std::tuple<TailTwo...>>::type>;
    };
    template <typename... TailTwo>
    struct FindFirstOfImpl<std::tuple<>, std::tuple<TailTwo...>> {
        using type = End;
    };

    /**
     * Implementation for the AdjacentFind algorithm
     */
    /**
     * Stop the iteration here, either of the below two cases should be hit in
     * most cases, when they are not hit then the default case will be this
     * one which ends the recursion by defining the type to be the End type
     */
    template <typename... Types>
    struct AdjacentFindImpl {
        using type = End;
    };
    /**
     * Keep going here, since you have not found types that are the same
     */
    template <typename First, typename Second, typename... Types>
    struct AdjacentFindImpl<First, Second, Types...> {
        using type = typename AdjacentFindImpl<Second, Types...>::type;
    };
    /**
     * Stop here because you have hit the point where you have found two of
     * the same types
     */
    template <typename First, typename... Types>
    struct AdjacentFindImpl<First, First, Types...> {
        using type = std::tuple<First, First, Types...>;
    };

    /**
     * Implementation for the Mismatch trait
     */
    template <typename TypesContainerOne, typename TypesContainerTwo>
    struct MismatchImpl;
    template <typename HeadOne, typename... TailOne,
              typename HeadTwo, typename... TailTwo>
    struct MismatchImpl<std::tuple<HeadOne, TailOne...>,
                        std::tuple<HeadTwo, TailTwo...>> {
        using type = std::pair<std::tuple<HeadOne, TailOne...>,
                               std::tuple<HeadTwo, TailTwo...>>;
    };
    template <typename HeadOne, typename... TailOne>
    struct MismatchImpl<std::tuple<HeadOne, TailOne...>, std::tuple<>> {
        using type = std::pair<std::tuple<HeadOne, TailOne...>,
                               End>;
    };
    template <typename HeadTwo, typename... TailTwo>
    struct MismatchImpl<std::tuple<>, std::tuple<HeadTwo, TailTwo...>> {
        using type = std::pair<End,
                               std::tuple<HeadTwo, TailTwo...>>;
    };
    template <>
    struct MismatchImpl<std::tuple<>, std::tuple<>> {
        using type = std::pair<End, End>;
    };
    template <typename HeadOne, typename... TailOne,
              typename... TailTwo>
    struct MismatchImpl<std::tuple<HeadOne, TailOne...>,
                        std::tuple<HeadOne, TailTwo...>> {
        using type = typename MismatchImpl<std::tuple<TailOne...>,
                                           std::tuple<TailTwo...>>::type;
    };

    /**
     * Implementation for the constexpr max trait
     */
    template <int... integers>
    struct MaxValueImpl : std::integral_constant<int, -1> {};
    template <int first>
    struct MaxValueImpl<first> : std::integral_constant<int, first> {};
    template <int first, int second, int... tail>
    struct MaxValueImpl<first, second, tail...> {
        static constexpr const int value = (first > second)
                ? MaxValueImpl<first, tail...>::value
                : MaxValueImpl<second, tail...>::value;
    };

    /**
     * Implementation for the type based Max trait, this follows the same
     * logic as the trait above, but just differently for types by using the
     * comparator
     */
    template <template <typename...> class Comparator, typename... Types>
    struct MaxTypeImpl {
        using type = End;
    };
    template <template <typename...> class Comparator, typename Head>
    struct MaxTypeImpl<Comparator, Head> {
        using type = Head;
    };
    template <template <typename...> class Comparator,
              typename First, typename Second, typename... Tail>
    struct MaxTypeImpl<Comparator, First, Second, Tail...> {
        using type = std::conditional_t<
            Comparator<First, Second>::value,
            typename MaxTypeImpl<Comparator, Second, Tail...>::type,
            typename MaxTypeImpl<Comparator, First, Tail...>::type>;
    };

    template <int... integers>
    struct MinImpl : std::integral_constant<int, -1> {};
    template <int first>
    struct MinImpl<first> : std::integral_constant<int, first> {};
    template <int first, int second, int... tail>
    struct MinImpl<first, second, tail...> {
        static constexpr const int value = (first < second)
                ? MinImpl<first, tail...>::value
                : MinImpl<second, tail...>::value;
    };


} // namespace detail

/**
 * @class AllOf
 *
 * A trait that returns true if the predicate returns true on all the types in
 * the type list, for example <std::is_reference, int&, double&> will return
 * true
 */
template <template <typename...> class Predicate, typename... Types>
struct AllOf {
    static constexpr const bool value
        = detail::AllOfImpl<Predicate, Types...>::value;
};

/**
 * @class AnyOf
 *
 * A trait that returns true if the predicate returns true on any of the types
 * in the type list, for example <std::is_reference, int&, double> will
 * return true
 */
template <template <typename...> class Predicate, typename... Types>
struct AnyOf {
    static constexpr const bool value
        = detail::AnyOfImpl<Predicate, Types...>::value;
};

/**
 * @class NoneOf
 *
 * A trait that returns a true if the predicate returns true for none of the
 * types in the type list, this is just the negation of AnyOf
 */
template <template <typename...> class Predicate, typename... Types>
struct NoneOf {
    static constexpr const bool value = !AnyOf<Predicate, Types...>::value;
};

/**
 * @class CountIf
 *
 * A trait that counts the number of times the predicate returns true on the
 * type list
 */
template <template <typename...> class Predicate, typename... Types>
struct CountIf {
    static constexpr const int value
        = detail::CountIfImpl<Predicate, Types...>::value;
};

/**
 * @class Mismatch
 *
 * A trait that returns the first types that are not the same in the two
 * provided type lists
 */
template <typename TypesContainerOne, typename TypesContainerTwo>
struct Mismatch {
    using type = typename detail::MismatchImpl<TypesContainerOne,
                                               TypesContainerTwo>::type;
};

/**
 * @class FindIf
 *
 * A trait that lets you find the first type for which the predicate returned
 * true, similar to std::find_if
 */
template <template <typename...> class Predicate, typename... Types>
struct FindIf {
    using type
        = typename detail::FindIfImpl<Predicate, Types...>::type;
};

/**
 * @class Find
 *
 * A trait that lets you find the type passed in, this is implemented using
 * the FindIf trait and a std::is_same predicate
 */
template <typename ToFind, typename... Types>
struct Find {
    using type = typename FindIf<Bind<std::is_same, ToFind>::template type,
                                 Types...>::type;
};

/**
 * @class FindIfNot
 *
 * A trait that lets you find the first type for which the predicate returned
 * false, similar to std::find_if_not
 */
template <template <typename...> class Predicate, typename... Types>
struct FindIfNot {
    using type
        = typename FindIf<Negate<Predicate>::template type, Types...>::type;
};

/**
 * @class FindFirstOf
 *
 * A trait that lets you find the first type in the first list that matches
 * any element in the second list
 */
template <typename TypesContainerOne, typename TypesContainerTwo>
struct FindFirstOf {
    using type = typename detail::FindFirstOfImpl<TypesContainerOne,
          TypesContainerTwo>::type;
};

/**
 * @class AdjacentFind
 *
 * A trait that lets you find the first time a type was repeated in a type
 * list, it returns the repeated type, if no type was repeated, then this will
 * return the End type
 */
template <typename... Types>
struct AdjacentFind {
    using type = typename detail::AdjacentFindImpl<Types...>::type;
};

/**
 * @class Max
 *
 * Determines the maximum of the given integral values.  If types are given
 * instead then the user must provide a comparator that can be used to compare
 * two different types and test which one is smaller, i.e. the predicate
 * should behave the same way as it does in the standard library, it should
 * be a logical less than operator, for example the type A is smaller than the
 * type B if Comparator<A, B>::value is true
 */
template <int... integers>
struct MaxValue {
    static constexpr const int value = detail::MaxValueImpl<integers...>::value;
};
template <template <typename...> class Comparator, typename... Types>
struct MaxType {
    using type = typename detail::MaxTypeImpl<Comparator, Types...>::type;
};

/**
 * @class Min
 *
 * Determines the maximum of the given integral values.  If types are given
 * instead then the user must provide a comparator that can be used to compare
 * two different types and test which one is smaller, i.e. the predicate
 * should behave the same way as it does in the standard library, it should
 * be a logical less than operator, for example the type A is smaller than the
 * type B if Comparator<A, B>::value is true
 */
template <int... integers>
struct MinValue {
    static constexpr const int value = detail::MinImpl<integers...>::value;
};
template <template <typename...> class Comparator, typename... Types>
struct MinType {
    using type
        = typename MaxType<Negate<Comparator>::template type, Types...>::type;
};

/**
 * Conventional value typedefs, these end in the suffix _v, this is keeping in
 * convention with the C++ standard library features post and including C++17
 */
template <template <typename...> class Predicate, typename... Types>
constexpr const bool AllOf_v = AllOf<Predicate, Types...>::value;
template <template <typename...> class Predicate, typename... Types>
constexpr const bool AnyOf_v = AnyOf<Predicate, Types...>::value;
template <template <typename...> class Predicate, typename... Types>
constexpr const bool NoneOf_v = NoneOf<Predicate, Types...>::value;
template <template <typename...> class Predicate, typename... Types>
constexpr const int CountIf_v = CountIf<Predicate, Types...>::value;
template <int... integers>
constexpr const int MaxValue_v = MaxValue<integers...>::value;
template <int... integers>
constexpr const int MinValue_v = MinValue<integers...>::value;

/**
 * Conventional typedefs, these end in the suffix _t, this is keeping in
 * convention with the C++ standard library features post and including C++17
 */
template <typename TypesContainerOne, typename TypesContainerTwo>
using Mismatch_t = typename Mismatch<TypesContainerOne, TypesContainerTwo>
    ::type;
template <template <typename...> class Predicate, typename... Types>
using FindIf_t = typename FindIf<Predicate, Types...>::type;
template <typename ToFind, typename... Types>
using Find_t = typename Find<ToFind, Types...>::type;
template <template <typename...> class Predicate, typename... Types>
using FindIfNot_t = typename FindIfNot<Predicate, Types...>::type;
template <typename TypesContainerOne, typename TypesContainerTwo>
using FindFirstOf_t = typename FindFirstOf<
    TypesContainerOne, TypesContainerTwo>::type;
template <typename... Types>
using AdjacentFind_t = typename AdjacentFind<Types...>::type;
template <template <typename...> class Comparator, typename... Types>
using MaxType_t = typename MaxType<Comparator, Types...>::type;
template <template <typename...> class Comparator, typename... Types>
using MinType_t = typename MinType<Comparator, Types...>::type;

/*******************************************************************************
 * Tests
 ******************************************************************************/
namespace detail { namespace test {
    template <typename ValueListOne, typename ValueListTwo>
    struct LessThanValueList;
    template <int value_one, int value_two>
    struct LessThanValueList<ValueList<value_one>, ValueList<value_two>> {
        static constexpr const bool value = value_one < value_two;
    };
}}

/**
 * Tests for AllOf
 */
static_assert(AllOf_v<std::is_reference>,
        "sharp::AllOf tests failed!");
static_assert(AllOf_v<std::is_reference, int&>,
        "sharp::AllOf tests failed!");
static_assert(!AllOf_v<std::is_reference, int&, double>,
        "sharp::AllOf tests failed!");
static_assert(!AllOf_v<std::is_reference, int, double&>,
        "sharp::AllOf tests failed!");
static_assert(AllOf_v<std::is_reference, int&, double&>,
        "sharp::AllOf tests failed!");

/**
 * Tests for AnyOf
 */
static_assert(!AnyOf_v<std::is_reference>,
        "sharp::AnyOf tests failed!");
static_assert(AnyOf_v<std::is_reference, int&>,
        "sharp::AnyOf tests failed!");
static_assert(AnyOf_v<std::is_reference, int&, double>,
        "sharp::AnyOf tests failed!");
static_assert(AnyOf_v<std::is_reference, int, double&>,
        "sharp::AnyOf tests failed!");
static_assert(!AnyOf_v<std::is_reference, int*, double*>,
        "sharp::AnyOf tests failed!");
static_assert(AnyOf_v<std::is_reference, int&, double&>,
        "sharp::AnyOf tests failed!");

/**
 * Tests for NoneOf
 */
static_assert(NoneOf_v<std::is_reference>,
        "sharp::NoneOf tests failed!");
static_assert(!NoneOf_v<std::is_reference, int&>,
        "sharp::NoneOf tests failed!");
static_assert(!NoneOf_v<std::is_reference, int&, double>,
        "sharp::NoneOf tests failed!");
static_assert(!NoneOf_v<std::is_reference, int, double&>,
        "sharp::NoneOf tests failed!");
static_assert(NoneOf_v<std::is_reference, int*, double*>,
        "sharp::NoneOf tests failed!");
static_assert(!NoneOf_v<std::is_reference, int&, double&>,
        "sharp::NoneOf tests failed!");

/**
 * Tests for CountIf
 */
static_assert(CountIf_v<std::is_reference> == 0,
        "sharp::CountIf tests failed!");
static_assert(CountIf_v<std::is_reference, int&> == 1,
        "sharp::CountIf tests failed!");
static_assert(CountIf_v<std::is_reference, int&, double> == 1,
        "sharp::CountIf tests failed!");
static_assert(CountIf_v<std::is_reference, int&, double&> == 2,
        "sharp::CountIf tests failed!");

/**
 * Tests for Max
 */
static_assert(MaxValue_v<> == -1, "sharp::Max tests failed!");
static_assert(MaxValue_v<1> == 1, "sharp::Max tests failed!");
static_assert(MaxValue_v<1, 2> == 2, "sharp::Max tests failed!");
static_assert(MaxValue_v<1, 2, 3> == 3, "sharp::Max tests failed!");
static_assert(MaxValue_v<-1, 2, 3> == 3, "sharp::Max tests failed!");

static_assert(std::is_same<MaxType_t<detail::test::LessThanValueList,
                                     ValueList<0>, ValueList<1>>,
                           ValueList<1>>::value, "sharp::Max tests failed!");
static_assert(std::is_same<MaxType_t<detail::test::LessThanValueList,
                                     ValueList<1>, ValueList<0>>,
                           ValueList<1>>::value, "sharp::Max tests failed!");

/**
 * Tests for Min
 */
static_assert(MinValue_v<> == -1, "sharp::Min tests failed!");
static_assert(MinValue_v<1> == 1, "sharp::Min tests failed!");
static_assert(MinValue_v<1, 2> == 1, "sharp::Min tests failed!");
static_assert(MinValue_v<1, 2, 3> == 1, "sharp::Min tests failed!");
static_assert(MinValue_v<-1, 2, 3> == -1, "sharp::Min tests failed!");
static_assert(std::is_same<MinType_t<detail::test::LessThanValueList,
                                     ValueList<0>, ValueList<1>>,
                           ValueList<0>>::value, "sharp::Min tests failed!");
static_assert(std::is_same<MinType_t<detail::test::LessThanValueList,
                                     ValueList<1>, ValueList<0>>,
                           ValueList<0>>::value, "sharp::Min tests failed!");

/**
 * Tests for Mismatch
 */
static_assert(std::is_same<Mismatch_t<std::tuple<int, double, char>,
                                      std::tuple<int, double, char*>>,
                           std::pair<std::tuple<char>, std::tuple<char*>>>
        ::value, "sharp::Mismatch tests failed!");
static_assert(std::is_same<Mismatch_t<std::tuple<>, std::tuple<>>,
                           std::pair<End, End>>::value,
        "sharp::Mismatch tests failed!");
static_assert(std::is_same<Mismatch_t<std::tuple<int, double>, std::tuple<>>,
                           std::pair<std::tuple<int, double>, End>>::value,
        "sharp::Mismatch tests failed!");
static_assert(std::is_same<Mismatch_t<std::tuple<>, std::tuple<int, double>>,
                           std::pair<End, std::tuple<int, double>>>::value,
        "sharp::Mismatch tests failed!");
static_assert(std::is_same<Mismatch_t<std::tuple<int, char*>,
                                      std::tuple<int, double>>,
                           std::pair<std::tuple<char*>, std::tuple<double>>>
        ::value, "sharp::Mismatch tests failed!");
static_assert(std::is_same<Mismatch_t<std::tuple<int, char*>,
                                      std::tuple<int, double, bool, char>>,
                           std::pair<std::tuple<char*>,
                                     std::tuple<double, bool, char>>>
        ::value, "sharp::Mismatch tests failed!");

/**
 * Tests for FindIf
 */
static_assert(std::is_same<FindIf_t<std::is_reference>, End>::value,
        "sharp::FindIt tests failed!");
static_assert(std::is_same<FindIf_t<std::is_reference, int, int&>,
        std::tuple<int&>>::value, "sharp::FindIf tests failed!");
static_assert(std::is_same<FindIf_t<std::is_reference, int*, int&>,
        std::tuple<int&>>::value, "sharp::FindIf tests failed!");
static_assert(std::is_same<FindIf_t<std::is_reference, double, int>, End>
        ::value, "sharp::FindIf tests failed!");
static_assert(std::is_same<FindIf_t<std::is_reference, double&, int>,
        std::tuple<double&, int>>::value, "sharp::FindIf tests failed!");

/**
 * Tests for Find
 */
static_assert(std::is_same<Find_t<int>, End>::value,
        "sharp::FindIt tests failed!");
static_assert(std::is_same<Find_t<int, double, int>, std::tuple<int>>::value,
        "sharp::FindIf tests failed!");
static_assert(std::is_same<Find_t<int, int, double>,
        std::tuple<int, double>>::value, "sharp::FindIf tests failed!");
static_assert(std::is_same<Find_t<int, double*, int>, std::tuple<int>>::value,
        "sharp::FindIf tests failed!");
static_assert(std::is_same<Find_t<int, double*, int, bool>,
                           std::tuple<int, bool>>::value,
        "sharp::FindIf tests failed!");

/**
 * Tests for FindIfNot
 */
static_assert(std::is_same<FindIfNot_t<std::is_reference>, End>::value,
        "sharp::FindIfNot tests failed!");
static_assert(std::is_same<FindIfNot_t<std::is_reference, int, int&>,
        std::tuple<int, int&>>::value, "sharp::FindItNot tests failed!");
static_assert(std::is_same<FindIfNot_t<std::is_reference, int*, int&>,
        std::tuple<int*, int&>>::value, "sharp::FindItNot tests failed!");
static_assert(std::is_same<FindIfNot_t<std::is_reference, int&, double, int>,
        std::tuple<double, int>>::value, "sharp::FindIfNot tests failed!");

/**
 * Tests for FindFirstOf
 */
static_assert(std::is_same<FindFirstOf_t<std::tuple<>, std::tuple<>>, End>
        ::value, "sharp::FindFirstOf tests failed!");
static_assert(std::is_same<FindFirstOf_t<std::tuple<int>, std::tuple<>>, End>
        ::value, "sharp::FindFirstOf tests failed!");
static_assert(std::is_same<FindFirstOf_t<std::tuple<>, std::tuple<int>>, End>
        ::value, "sharp::FindFirstOf tests failed!");
static_assert(std::is_same<FindFirstOf_t<std::tuple<int, double>,
                                         std::tuple<>>, End>
        ::value, "sharp::FindFirstOf tests failed!");
static_assert(std::is_same<FindFirstOf_t<std::tuple<>,
                                         std::tuple<int, double>>, End>
        ::value, "sharp::FindFirstOf tests failed!");
static_assert(std::is_same<FindFirstOf_t<std::tuple<int, double>,
                                         std::tuple<char, double>>,
                                         std::tuple<double>>
        ::value, "sharp::FindFirstOf tests failed!");
static_assert(std::is_same<FindFirstOf_t<std::tuple<int, double*>,
                                         std::tuple<char, double>>, End>
        ::value, "sharp::FindFirstOf tests failed!");
static_assert(std::is_same<FindFirstOf_t<std::tuple<int, double*>,
                                         std::tuple<int, double>>,
                                         std::tuple<int, double*>>
        ::value, "sharp::FindFirstOf tests failed!");

/**
 * Tests for AdjacentFind
 */
static_assert(std::is_same<AdjacentFind_t<int, double, char>, End>::value,
        "sharp::AdjacentFind tests failed!");
static_assert(std::is_same<AdjacentFind_t<int, int, char>,
        std::tuple<int, int, char>>::value,
        "sharp::AdjacentFind tests failed!");
static_assert(std::is_same<AdjacentFind_t<char, int, int>,
        std::tuple<int, int>>::value, "sharp::AdjacentFind tests failed!");
static_assert(std::is_same<AdjacentFind_t<int*, double&, int*>, End>::value,
        "sharp::AdjacentFind tests failed!");

} // namespace sharp
