/*
 * @file Instantiation.hpp
 * @author Aaryaman Sagar
 *
 * This file contains a type trait for determining whether a class template is
 * an instantiation of another, for example std::list<int> is an instantiation
 * of std::list
 */

#pragma once

#include <type_traits>

namespace sharp {

/**
 * @class IsInstantiationOf
 *
 * This class is a template type that derives from std::integral_constant to
 * give it constant constexpr behavior as a compile time template constant
 * that evaluates to true when the type on the left is an instantiatation of
 * the type on the right, for example
 *
 *      cout << std::boolalpha << IsInstantiationOf_v<std::list<int>, std::list>
 *
 * will output a "true" to standard out
 */
/**
 * Default non specialized implementaton for when the type on the left is not
 * an instantiation of the template argument on the right, this uses the
 * nested template syntax
 */
template <typename Type, template <typename...> class InstantiatedType>
struct IsInstantiationOf : public std::integral_constant<bool, false> {};

/**
 * Specialized implementation for when the type on the left is an
 * instantiation of the type on the right
 *
 * Keep in mind that the order of arguments does in the initial template list
 * does not matter for using this trait, the real order in specializations is
 * maintained with the specialized list after the name of the struct after the
 * first line below this comment block
 */
template <template <typename...> class InstantiatedType, typename... Args>
struct IsInstantiationOf<InstantiatedType<Args...>, InstantiatedType>
    : public std::integral_constant<bool, true> {};

/**
 * Convenience variable template for uniformity with the standard library
 * value traits, the _v is added to all value traits in the C++17 standard
 */
template <typename Type, template <typename...> class InstantiatedType>
constexpr bool IsInstantiationOf_v
    = IsInstantiationOf<Type, InstantiatedType>::value;

/**
 * A temporary Vector class used to test the IsInstantiationOf trait
 */
namespace detail { namespace test {
    template <typename...>
    class Vector {};
}}

/**
 * Test suite for this header, will be ran whenever someone includes this
 */
static_assert(IsInstantiationOf_v<detail::test::Vector<int>,
        detail::test::Vector>, "sharp::IsInstantiationOf tests failed!");
static_assert(IsInstantiationOf_v<detail::test::Vector<int, double>,
        detail::test::Vector>, "sharp::IsInstantiationOf tests failed!");

} // namespace sharp
