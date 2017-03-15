/**
 * @file TypeAlgorithm.hpp
 * @author Aaryaman Sagar
 *
 * This file contains algorithms that can be executed on a template pack,
 * including things similar to std::any_of std::all_of std::max and so on
 */

#pragma once

#include <type_traits>

namespace sharp {

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
 * Default value typedefs, these end in the suffix _v, this is keeping in
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

} // namespace sharp
