#pragma once

#include <sharp/Invoke/Invoke.hpp>

#include <tuple>
#include <type_traits>
#include <utility>

namespace sharp {

/**
 * Placeholder optional and variant classes till std::optional and
 * std::variant are available
 */
template <typename T>
class OptionalType {};
template <typename...>
class VariantType {};

namespace invoke_detail {

} // namespace invoke_detail

template <typename Optional = OptionalType, typename Variant = VariantType,
          typename Func, typename... Args>
decltype(auto) invoke(Func&& func, Args&&... args) {

    // store the args in a tuple
    auto args = std::forward_as_tuple(std::forward<Args>(args)...);

    // then forward the args to the implementation
    return invoke_detail::invoke_impl(std::forward<Func>(func),
            std::make_index_sequence<sizeof...(Args)>{}, std::move(args));
}

} // namespace sharp
