/**
 * @file Conjunction.hpp
 * @author Aaryaman Sagar
 *
 * An implementation of conjunction until fold expressions and c++17
 * std::conjunction are supported
 */

#pragma once

namespace sharp {

/**
 * TODO deprecate this
 */
template <class...>
struct conjunction : std::true_type { };
template <class B1>
struct conjunction<B1> : B1 { };
template <class B1, class... Bn>
struct conjunction<B1, Bn...>
    : std::conditional_t<bool(B1::value), conjunction<Bn...>, B1> {};

template <typename... B>
inline constexpr bool conjunction_v = conjunction<B...>::value;

} // namespace sharp
