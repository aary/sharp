/**
 * @file TypeIndexing.hpp
 * @author Aaryaman Sagar
 *
 * This file contains a trait that lets you index into a type list and get the
 * type at the specified index location
 */

#pragma once

#include <type_traits>

namespace sharp {

namespace detail {
    template <int argument_index, typename... Args>
    struct TypeAtIndexImpl;

    /**
     * Keep going and find until the argument index value has hit 0, at that
     * point we know that the current head of the type list has to be the type
     * that was requested
     */
    template <int argument_index, typename Head, typename... Tail>
    struct TypeAtIndexImpl<argument_index, Head, Tail...>
            : TypeAtIndexImpl<argument_index - 1, Tail...> {};

    /**
     * Stop here, since the argument_index value is a 0, we cannot go further
     */
    template <typename Head, typename... Tail>
    struct TypeAtIndexImpl<0, Head, Tail...> {
        using type = Head;
    };
} // namespace detail

/**
 * @class TypeAtIndex
 *
 * A class that allows you to query a variadic template pack and get the type
 * at the specified index in the pack, this could not be implemented as a
 * simple wrapper around std::tuple and std::get<>() because std::get<>()
 * returns a refernece type and it would be more work than not to go through
 * the tuple and see if a type like that exists in the tuple and then remove
 * the reference
 */
template <int argument_index, typename... Args>
struct TypeAtIndex {

    /**
     * Fire dem assertions
     */
    static_assert(sizeof...(Args) > 0, "Can only query a non 0 sized pack");
    static_assert(argument_index >= 0, "The type index cannot be negative");
    static_assert(argument_index < sizeof...(Args),
            "Type index must be in the range [0, sizeof(Args)...)");

    using type = typename detail::TypeAtIndexImpl<argument_index, Args...>
        ::type;
};


/**
 * Convenience template for uniformity with the standard library type traits,
 * the _t is added to all type traits in the C++14 standard
 */
template <int argument_index, typename... Args>
using TypeAtIndex_t = typename TypeAtIndex<argument_index, Args...>::type;

static_assert(std::is_same<TypeAtIndex_t<0, int>, int>::value,
        "sharp::TypeAtIndex_t tests failed!");
static_assert(std::is_same<TypeAtIndex_t<1, int, double>, double>::value,
        "sharp::TypeAtIndex_t tests failed!");
static_assert(std::is_same<TypeAtIndex_t<0, int&, double, char>, int&>::value,
        "sharp::TypeAtIndex_t tests failed!");
static_assert(std::is_same<TypeAtIndex_t<2, int&, double, char*>, char*>::value,
        "sharp::TypeAtIndex_t tests failed!");
static_assert(std::is_same<TypeAtIndex_t<0, const int&>, const int&>::value,
        "sharp::TypeAtIndex_t tests failed!");

} // namespace sharp
