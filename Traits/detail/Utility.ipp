/**
 * @file Utility.ipp
 * @author Aaryaman Sagar (rmn100@gmail.com)
 *
 * This file contains the implementations of the functions provided in the
 * utilities module
 */

#pragma once

#include <tuple>
#include <type_traits>
#include <utility>

#include <sharp/Traits/detail/Utility.hpp>

namespace sharp {

namespace detail {

    /**
     * Implementation of the for_each_tuple function
     */
    template <int current, int last>
    struct ForEachTupleImpl {

        template <typename TupleType, typename Func>
        static void impl(TupleType&& tup, Func& func) {

            // call the object at the given index
            func(std::get<current>(std::forward<TupleType>(tup)));

            // and then recurse
            ForEachTupleImpl<current + 1, last>::impl(
                    std::forward<TupleType>(tup), func);
        }
    };
    /**
     * No-op on last
     */
    template <int last>
    struct ForEachTupleImpl<last, last> {
        template <typename TupleType, typename Func>
        static void impl(TupleType&&, Func&) {}
    };
} // namespace detail

template <typename TupleType, typename Func,
          detail::EnableIfTupleInstantiation<TupleType>*>
Func for_each_tuple(TupleType&& tup, Func func) {

    // call the implementation function and then return the functor, similar
    // to std::for_each
    detail::ForEachTupleImpl<
        0, std::tuple_size<std::decay_t<TupleType>>::value>::impl(
                std::forward<TupleType>(tup), func);
    return func;
}

} // namespace sharp
