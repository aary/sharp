/**
 * @file VoidT.hpp
 * @author Aaryaman Sagar
 *
 * Contains a void_t implementation that works on most systems
 */

#pragma once

namespace sharp {

namespace detail {
    template <typename... Types> struct MakeVoid {
        typedef void type;
    };
} // namespace detail

template <typename... Types>
using void_t = typename detail::MakeVoid<Types...>::type;

} // namespace sharp
