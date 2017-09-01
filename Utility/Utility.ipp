#pragma once

#include <sharp/Utility/Utility.hpp>

#include <type_traits>
#include <utility>

namespace sharp {

namespace utility_detail {

    /**
     * Checks if an object of the class is constructible from an rvalue of the
     * same class
     */
    template <typename Type>
    using EnableIfRValueConstructible = std::enable_if_t<
        std::is_move_constructible<Type>::value>;
    template <typename Type>
    using EnableIfNotRValueConstructible = std::enable_if_t<
        !std::is_move_constructible<Type>::value>;

} // namespace detail


template <typename TypeToMatch, typename Type>
decltype(auto) match_forward(std::remove_reference_t<Type>& instance) {


    // is the type TypeToMatch a const reference (or just const) type?
    constexpr auto is_const = std::is_const<
        std::remove_reference_t<TypeToMatch>>::value;

    // get the type to cast to based on the reference category of the
    // TypeToMatch
    using TypeToCast = std::conditional_t<
        std::is_lvalue_reference<TypeToMatch>::value,
        std::conditional_t<
            is_const,
            const std::remove_reference_t<Type>&,
            std::remove_reference_t<Type>&>,
        std::conditional_t<
            is_const,
            const std::remove_reference_t<Type>&&,
            std::remove_reference_t<Type>&&>>;

    // then cast to that type and then return
    return static_cast<TypeToCast>(instance);
}

template <typename TypeToMatch, typename Type>
decltype(auto) match_forward(std::remove_reference_t<Type>&& instance) {

    // if the instance is an rvalue reference then casting that to an lvalue
    // might cause a dangling reference based on the value category of the
    // original expression so assert against that
    static_assert(!std::is_lvalue_reference<TypeToMatch>::value,
            "Can not forward an rvalue as an lvalue");

    // get the type to cast to with the right const-ness
    using TypeToCast = std::conditional_t<
        std::is_const<std::remove_reference_t<TypeToMatch>>::value,
        const std::remove_reference_t<Type>&&,
        std::remove_reference_t<Type>&&>;

    // then cast the expression to an rvalue and return
    return static_cast<TypeToCast>(instance);
}

template <typename Type,
          utility_detail::EnableIfRValueConstructible<Type>* = nullptr>
decltype(auto) move_if_movable(Type&& object) {
    return std::move(object);
}

template <typename Type,
          utility_detail::EnableIfNotRValueConstructible<Type>* = nullptr>
decltype(auto) move_if_movable(const Type& object) {
    return object;
}

template <template <typename> class Base, typename Derived>
Derived& Crtp<Base<Derived>>::instance() {
    return static_cast<Derived&>(*this);
}

template <template <typename> class Base, typename Derived>
const Derived& Crtp<Base<Derived>>::instance() const {
    return static_cast<const Derived&>(*this);
}

} // namespace sharp
