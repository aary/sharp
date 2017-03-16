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
    template <template <typename...> class Predicate, typename... TypeList>
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
    template <template <typename...> class Predicate, typename... TypeList>
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
    template <template <typename...> class Predicate, typename... TypeList>
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
    template <template <typename...> class Predicate, typename... TypeList>
    struct FindIfImpl {
        using type = End;
    };
    template <template <typename...> class Predicate,
              typename Head, typename... Tail>
    struct FindIfImpl<Predicate, Head, Tail...> {
        using type = std::conditional_t<
            Predicate<Head>::value,
            Head,
            typename FindIfImpl<Predicate, Tail...>::type>;
    };

    /**
     * Implementation for the FindFirstOfTrait
     */
    template <typename TypeListContainerOne, typename TypeListContainerTwo>
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
            HeadOne,
            typename FindFirstOfImpl<std::tuple<TailOne...>,
                                     std::tuple<TailTwo...>>::type>;
    };
    template <typename... TailTwo>
    struct FindFirstOfImpl<std::tuple<>, std::tuple<TailTwo...>> {
        using type = End;
    };
}

/**
 * @class AllOf
 *
 * A trait that returns true if the predicate returns true on all the types in
 * the type list, for example <std::is_reference, int&, double&> will return
 * true
 */
template <template <typename...> class Predicate, typename... TypeList>
struct AllOf {
    static constexpr const bool value
        = detail::AllOfImpl<Predicate, TypeList...>::value;
};

/**
 * @class AnyOf
 *
 * A trait that returns true if the predicate returns true on any of the types
 * in the type list, for example <std::is_reference, int&, double> will
 * return true
 */
template <template <typename...> class Predicate, typename... TypeList>
struct AnyOf {
    static constexpr const bool value
        = detail::AnyOfImpl<Predicate, TypeList...>::value;
};

/**
 * @class NoneOf
 *
 * A trait that returns a true if the predicate returns true for none of the
 * types in the type list, this is just the negation of AnyOf
 */
template <template <typename...> class Predicate, typename... TypeList>
struct NoneOf {
    static constexpr const bool value = !AnyOf<Predicate, TypeList...>::value;
};

/**
 * @class CountIf
 *
 * A trait that counts the number of times the predicate returns true on the
 * type list
 */
template <template <typename...> class Predicate, typename... TypeList>
struct CountIf {
    static constexpr const int value
        = detail::CountIfImpl<Predicate, TypeList...>::value;
};

/**
 * @class FindIf
 *
 * A trait that lets you find the first type for which the predicate returned
 * true, similar to std::find_if
 */
template <template <typename...> class Predicate, typename... TypeList>
struct FindIf {
    using type
        = typename detail::FindIfImpl<Predicate, TypeList...>::type;
};

/**
 * @class Find
 *
 * A trait that lets you find the type passed in, this is implemented using
 * the FindIf trait and a std::is_same predicate
 */
template <typename ToFind, typename... TypeList>
struct Find {
    using type = typename FindIf<Bind<std::is_same, ToFind>::template type,
                                 TypeList...>::type;
};

/**
 * @class FindIfNot
 *
 * A trait that lets you find the first type for which the predicate returned
 * false, similar to std::find_if_not
 */
template <template <typename...> class Predicate, typename... TypeList>
struct FindIfNot {
    using type
        = typename FindIf<Negate<Predicate>::template type, TypeList...>::type;
};

/**
 * @class FindFirstOf
 *
 * A trait that lets you find the first type in the first list that matches
 * any element in the second list
 */
template <typename TypeListContainerOne, typename TypeListContainerTwo>
struct FindFirstOf {
    using type = typename detail::FindFirstOfImpl<TypeListContainerOne,
          TypeListContainerTwo>::type;
};

/**
 * Conventional value typedefs, these end in the suffix _v, this is keeping in
 * convention with the C++ standard library features post and including C++17
 */
template <template <typename...> class Predicate, typename... TypeList>
constexpr const bool AllOf_v = AllOf<Predicate, TypeList...>::value;
template <template <typename...> class Predicate, typename... TypeList>
constexpr const bool AnyOf_v = AnyOf<Predicate, TypeList...>::value;
template <template <typename...> class Predicate, typename... TypeList>
constexpr const bool NoneOf_v = NoneOf<Predicate, TypeList...>::value;
template <template <typename...> class Predicate, typename... TypeList>
constexpr const int CountIf_v = CountIf<Predicate, TypeList...>::value;

/**
 * Conventional typedefs, these end in the suffix _t, this is keeping in
 * convention with the C++ standard library features post and including C++17
 */
template <template <typename...> class Predicate, typename... TypeList>
using FindIf_t = typename FindIf<Predicate, TypeList...>::type;
template <typename ToFind, typename... TypeList>
using Find_t = typename Find<ToFind, TypeList...>::type;
template <template <typename...> class Predicate, typename... TypeList>
using FindIfNot_t = typename FindIfNot<Predicate, TypeList...>::type;
template <typename TypeListContainerOne, typename TypeListContainerTwo>
using FindFirstOf_t = typename FindFirstOf<
    TypeListContainerOne, TypeListContainerTwo>::type;

/*******************************************************************************
 * Tests
 ******************************************************************************/
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
static_assert(!!NoneOf_v<std::is_reference>,
        "sharp::NoneOf tests failed!");
static_assert(!NoneOf_v<std::is_reference, int&>,
        "sharp::NoneOf tests failed!");
static_assert(!NoneOf_v<std::is_reference, int&, double>,
        "sharp::NoneOf tests failed!");
static_assert(!NoneOf_v<std::is_reference, int, double&>,
        "sharp::NoneOf tests failed!");
static_assert(!!NoneOf_v<std::is_reference, int*, double*>,
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
 * Tests for FindIf
 */
static_assert(std::is_same<FindIf_t<std::is_reference>, End>::value,
        "sharp::FindIt tests failed!");
static_assert(std::is_same<FindIf_t<std::is_reference, int, int&>, int&>::value,
        "sharp::FindIf tests failed!");
static_assert(std::is_same<FindIf_t<std::is_reference, int*, int&>, int&>
        ::value, "sharp::FindIf tests failed!");
static_assert(std::is_same<FindIf_t<std::is_reference, double, int>, End>
        ::value, "sharp::FindIf tests failed!");

/**
 * Tests for Find
 */
static_assert(std::is_same<Find_t<int>, End>::value,
        "sharp::FindIt tests failed!");
static_assert(std::is_same<Find_t<int, double, int>, int>::value,
        "sharp::FindIf tests failed!");
static_assert(std::is_same<Find_t<int, int, double>, int>::value,
        "sharp::FindIf tests failed!");
static_assert(std::is_same<Find_t<int, double*, int>, int>::value,
        "sharp::FindIf tests failed!");

/**
 * Tests for FindIfNot
 */
static_assert(std::is_same<FindIfNot_t<std::is_reference>, End>::value,
        "sharp::FindIfNot tests failed!");
static_assert(std::is_same<FindIfNot_t<std::is_reference, int, int&>, int>
        ::value, "sharp::FindItNot tests failed!");
static_assert(std::is_same<FindIfNot_t<std::is_reference, int*, int&>, int*>
        ::value, "sharp::FindItNot tests failed!");
static_assert(std::is_same<FindIfNot_t<std::is_reference, double, int>, double>
        ::value, "sharp::FindIfNot tests failed!");

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
                                         std::tuple<char, double>>, double>
        ::value, "sharp::FindFirstOf tests failed!");
static_assert(std::is_same<FindFirstOf_t<std::tuple<int, double*>,
                                         std::tuple<char, double>>, End>
        ::value, "sharp::FindIfNot tests failed!");

} // namespace sharp
