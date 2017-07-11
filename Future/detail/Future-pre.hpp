/**
 * @file Future-pre.hpp
 * @author Aaryaman Sagar
 *
 * This file contains boring metaprogramming facilities that are used by the
 * public interface of future
 */

#pragma once

#include <sharp/Traits/Traits.hpp>

#include <type_traits>

namespace sharp {

/**
 * Forward declaration of Future for metaprogramming purposes
 */
template <typename Type>
class Future;
template <typename Type>
class SharedFuture;

namespace detail {

    /**
     * Enables the template in whatever way this might be used if the return
     * type of the functor is a future
     */
    template <typename Func, typename Type>
    using EnableIfReturnsFuture = std::enable_if_t<
        sharp::IsInstantiationOf_v<
            decltype(std::declval<Func>()(std::declval<Future<Type>>())),
            Future>>;
    template <typename Func, typename Type>
    using EnableIfDoesNotReturnFuture = std::enable_if_t<
        !sharp::IsInstantiationOf_v<
            decltype(std::declval<Func>()(std::declval<Future<Type>>())),
            Future>>;
    template <typename Func, typename Type>
    using EnableIfReturnsFutureShared = std::enable_if_t<
        sharp::IsInstantiationOf_v<
            decltype(std::declval<Func>()(std::declval<SharedFuture<Type>>())),
            Future>>;
    template <typename Func, typename Type>
    using EnableIfDoesNotReturnFutureShared = std::enable_if_t<
        !sharp::IsInstantiationOf_v<
            decltype(std::declval<Func>()(std::declval<SharedFuture<Type>>())),
            Future>>;

} // namespace detail

} // namespace sharp
