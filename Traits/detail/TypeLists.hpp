/**
 * @file TypeIndexing.hpp
 * @author Aaryaman Sagar
 *
 * This file contains traits that you can use to interact with type lists,
 * this includes querying type lists for a particular type, getting a type at
 * the specified index, checking if the type list is unique, etc.
 */

#pragma once

#include <type_traits>

namespace sharp {

namespace detail {
    template <int argument_index, typename... TypeList>
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


    /**
     * The implementation struct by itself exposes the false value back to the
     * caller, but the specializations come in and do the work in the right
     * cases most of the time, whenever sizeof...(Args) > 0.
     */
    template <typename ToQuery, typename... Args>
    struct TypeExistsImpl : std::integral_constant<bool, false> {
        /**
         * Since this should never be called when the length of Args is anything
         * other than 0, assert that in the body
         */
        static_assert(sizeof...(Args) == 0,
                "detail::TypeExistsImpl instantiated with non 0 length pack");
    };

    /**
     * Keep going and check if the type exists in the rest of the tail
     */
    template <typename ToQuery, typename Head, typename... Tail>
    struct TypeExistsImpl<ToQuery, Head, Tail...> {
        static constexpr const bool value = false
            || TypeExistsImpl<ToQuery, Tail...>::value;
    };

    /**
     * Dont have to keep going anymore, return a true and that will be
     * reflected up the "compile time call stack"
     */
    template <typename ToQuery, typename... Tail>
    struct TypeExistsImpl<ToQuery, ToQuery, Tail...>
            : std::integral_constant<bool, true> {};

    /**
     * Implementation trait for FindType, this starts off at 0 and then
     * continues through the type list until it finds the given type, as a
     * result the index of the type is going to be the first location where
     * a matching type is found, for example the location of an int in
     * <int, double, int, char> is 0 and not 2
     */
    template <typename ToQuery, typename... TypeList>
    struct FindTypeImpl
            : std::integral_constant<int, 0> {

        static_assert(sizeof...(TypeList) == 0,
                "detail::FindTypeImpl instantiated with non 0 length pack");
    };
    template <typename ToQuery, typename Head, typename... TypeList>
    struct FindTypeImpl<ToQuery, Head, TypeList...> {
        static constexpr const int value =
            1 + FindTypeImpl<ToQuery, TypeList...>::value;
    };
    template <typename ToQuery, typename... TypeList>
    struct FindTypeImpl<ToQuery, ToQuery, TypeList...>
            : std::integral_constant<int, 0> {};

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
template <int argument_index, typename... TypeList>
struct TypeAtIndex {

    /**
     * Fire dem assertions
     */
    static_assert(sizeof...(TypeList) > 0, "Can only query a non 0 sized pack");
    static_assert(argument_index >= 0, "The type index cannot be negative");
    static_assert(argument_index < sizeof...(TypeList),
            "Type index must be in the range [0, sizeof(TypeList)...)");

    using type = typename detail::TypeAtIndexImpl<argument_index, TypeList...>
        ::type;
};

/**
 * @class TypeExists
 *
 * A querying template that can be used to query whether a type exists in a
 * type list
 */
template <typename ToQuery, typename... TypeList>
struct TypeExists {
    static constexpr const bool value
        = detail::TypeExistsImpl<ToQuery, TypeList...>::value;
};

/**
 * @class FindType
 *
 * A trait that lets you inspect a type set and determine the index of the
 * type passed, for example the indices of int, bool and char in
 * <int, bool char> are 0, 1 and 2 respectively, if the type does not exist in
 * the template pack then the value of the trait will be the sizeof the
 * type list
 */
template <typename ToQuery, typename... TypeList>
struct FindType {
    static constexpr const int value
        = detail::FindTypeImpl<ToQuery, TypeList...>::value;
};

/**
 * Convenience template for uniformity with the standard library type traits,
 * the _t is added to all type traits in the C++14 standard
 */
template <int argument_index, typename... TypeList>
using TypeAtIndex_t = typename TypeAtIndex<argument_index, TypeList...>::type;

/**
 * Convenience template for uniformity with the standard library type traits,
 * the _v is added to all value traits in the C++ standard after and including
 * C++17
 */
template <typename ToQuery, typename... TypeList>
constexpr bool TypeExists_v = TypeExists<ToQuery, TypeList...>::value;
template <typename ToQuery, typename... TypeList>
constexpr int FindType_v = FindType<ToQuery, TypeList...>::value;

/*******************************************************************************
 * Tests
 ******************************************************************************/
/**
 * Tests for TypeAtIndex
 */
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

/**
 * Tests for TypeExists
 */
static_assert(!TypeExists_v<int>, "sharp::TypeExists tests failed!");
static_assert(TypeExists_v<int, int, double, char>,
        "sharp::TypeExists tests failed!");
static_assert(!TypeExists_v<int*, int, double, char>,
        "sharp::TypeExists tests failed!");
static_assert(!TypeExists_v<const int, int, double, char>,
        "sharp::TypeExists tests failed!");
static_assert(!TypeExists_v<volatile int, int, double, char>,
        "sharp::TypeExists tests failed!");
static_assert(!TypeExists_v<int&, int, double, char>,
        "sharp::TypeExists tests failed!");
static_assert(!TypeExists_v<int&, int, double, char>,
        "sharp::TypeExists tests failed!");
static_assert(TypeExists_v<int, char, double, int>,
        "sharp::TypeExists tests failed!");
static_assert(TypeExists_v<int, int>,
        "sharp::TypeExists tests failed!");
static_assert(TypeExists_v<int, char, int>,
        "sharp::TypeExists tests failed!");
static_assert(TypeExists_v<int, int, char>,
        "sharp::TypeExists tests failed!");
static_assert(TypeExists_v<int, double, int, char>,
        "sharp::TypeExists tests failed!");
static_assert(TypeExists_v<int, int>,
        "sharp::TypeExists tests failed!");
static_assert(TypeExists_v<int, int, int>,
        "sharp::TypeExists tests failed!");
static_assert(TypeExists_v<int, int, int, int>,
        "sharp::TypeExists tests failed!");

/**
 * Tests for FindType
 */
static_assert(FindType_v<int> == 0, "sharp::FindType tests failed!");
static_assert(FindType_v<int, char, int> == 1,
        "sharp::FindType tests failed!");
static_assert(FindType_v<int, int, char> == 0,
        "sharp::FindType tests failed!");
static_assert(FindType_v<int, char, double> == 2,
        "sharp::FindType tests failed!");
static_assert(FindType_v<int, char, double, int> == 2,
        "sharp::FindType tests failed!");
static_assert(FindType_v<int, int, double, char> == 0,
        "sharp::FindType tests failed!");
static_assert(FindType_v<int&, int, int&, double, char> == 1,
        "sharp::FindType tests failed!");
} // namespace sharp
