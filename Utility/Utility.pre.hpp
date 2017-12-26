/**
 * @file Utility.pre.hpp
 * @author Aaryaman Sagar
 *
 * Predeclarations and stuff for Utility.hpp
 */
#pragma once

#include <sharp/Portability/Portability.hpp>

#include <type_traits>

namespace sharp {
namespace utility_detail {

/**
 * Detects if Other is a cv-ref qualified version of Self
 */
template <typename Other, typename Self,
          typename = std::decay_t<Other>>
class IsSelf : public std::false_type {};
template <typename Other, typename Self>
class IsSelf<Other, Self, Self> : public std::true_type {};

/**
 * Detects if other is an instantiation of std::in_place_t
 */
template <typename Other, typename = std::decay_t<Other>>
class IsStdInPlace : public std::false_type {};
template <typename Other>
class IsStdInPlace<Other, std::in_place_t> : public std::true_type {};

/**
 * Enable if Other is neither self nor an instantiation of std::in_place_t
 */
template <typename Other, typename Self>
using EnableIfNotSelfAndNotInPlace = std::enable_if_t<
    !IsSelf<Other, Self>::value && !IsStdInPlace<Other>::value>;

} // namespace utility_detail
} // namespace sharp
