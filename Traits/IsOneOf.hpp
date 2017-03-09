/**
 * @file IsOneOf.hpp
 * @author Aaryaman Sagar
 *
 * This file contains a trait that can be used for variadic type
 * introspection of equality among several types.  In particular it can check
 * to see if a type is one of the types passed in a parameter pack to the
 * trait
 */

#pragma once

#include <type_traits>

namespace sharp {

namespace detail {

    /**
     * An imeplementation specialization trait, these specializations can be
     * used to unpack a parameter pack and see if one of the types match the
     * type that is being queried for
     */
    template <typename...>
    struct IsOneOfImpl;

    /**
     * Keep expanding in this one
     */
    template <typename ToCheck, typename Head, typename... Tail>
    struct IsOneOfImpl<ToCheck, Head, Tail...>
        : std::integral_constant<bool,
            false || IsOneOfImpl<ToCheck, Tail...>::value> {};

    /**
     * Stop here because once you find one that is similar then you can stop
     */
    template <typename ToCheck, typename... Tail>
    struct IsOneOfImpl<ToCheck, ToCheck, Tail...>
        : std::integral_constant<bool, true> {};
}

/**
 * The main trait just uses the implementaion trait after checking to make
 * sure that the AmongstThese parameter pack contains at least one type
 */
template <typename ToCheck, typename... AmongstThese>
struct IsOneOf : detail::IsOneOfImpl<ToCheck, AmongstThese...> {
    static_assert(sizeof...(AmongstThese) >= 1,
            "Need at least one type to query against with IsOneOf");
};

/**
 * Convenience variable template for uniformity with the standard library
 * value traits, the _v is added to all value traits in the C++17 standard
 */
template <typename ToCheck, typename... AmongstThese>
constexpr bool IsOneOf_v = IsOneOf<ToCheck, AmongstThese...>::value;

/**
 * Unit tests, these will be compiled and checked at compile time since they
 * are all static_asserts
 */
static_assert(IsOneOf_v<int, int>, "sharp::IsOneOf tests failed!");
static_assert(IsOneOf_v<int, char, bool, int>, "sharp::IsOneOf tests failed!");
static_assert(IsOneOf_v<int, bool, int>, "sharp::IsOneOf tests failed!");
static_assert(IsOneOf_v<int, int, bool>, "sharp::IsOneOf tests failed!");
static_assert(IsOneOf_v<int, int, char, bool>, "sharp::IsOneOf tests failed!");

} // namespace sharp
