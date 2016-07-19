/*
 * @file Unqualified.hpp
 * @author Aaryaman Sagar (rmn100@gmail.com)
 *
 * This file contains a simple module that removes all const, volatile and
 * reference qualifiers from a type and returns its "unqualified" type back to
 * the user
 */

#pragma once

#include <string>
#include <type_traits>

/**
 * @class Unqualified
 *
 * A template utility that removes all reference, const and volatile
 * qualifiers from a type
 */
template <typename Type>
struct Unqualified {
    using type = typename std::remove_cv<
        typename std::remove_reference<Type>::type>::type;
};

/**
 * @alias Unqualified_t
 *
 * A templated typedef that returns an expression equivalent to the
 * following
 *
 *      typename Unqualified<Type>::type;
 */
template <typename Type>
using Unqualified_t = typename Unqualified<Type>::type;


/**
 * Unit tests
 */
static_assert(std::is_same<Unqualified_t<const int&>, int>::value,
        "TypeTraits.hpp: Unqualified test failed");
static_assert(std::is_same<Unqualified_t<const int>, int>::value,
        "TypeTraits.hpp: Unqualified test failed");
static_assert(std::is_same<Unqualified_t<int&>, int>::value,
        "TypeTraits.hpp: Unqualified test failed");
static_assert(std::is_same<Unqualified_t<int const&>, int>::value,
        "TypeTraits.hpp: Unqualified test failed");
static_assert(std::is_same<Unqualified_t<int&&>, int>::value,
        "TypeTraits.hpp: Unqualified test failed");
static_assert(std::is_same<Unqualified_t<const std::string&>,
        std::string>::value,
        "TypeTraits.hpp: Unqualified test failed");
static_assert(std::is_same<Unqualified_t<const std::string>,
        std::string>::value,
        "TypeTraits.hpp: Unqualified test failed");
static_assert(std::is_same<Unqualified_t<std::string&>,
        std::string>::value,
        "TypeTraits.hpp: Unqualified test failed");
static_assert(std::is_same<Unqualified_t<std::string const&>,
        std::string>::value,
        "TypeTraits.hpp: Unqualified test failed");
static_assert(std::is_same<Unqualified_t<std::string&&>,
        std::string>::value,
        "TypeTraits.hpp: Unqualified test failed");
