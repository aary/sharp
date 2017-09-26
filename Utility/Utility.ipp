#pragma once

#include <sharp/Utility/Utility.hpp>

#include <type_traits>
#include <utility>
#include <functional>

namespace sharp {

namespace utility_detail {

    /**
     * Concept-ish checks
     */
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
    /**
     * Concept check to make sure that the two pointer types being passed to
     * the LessPtr point to types that are comparable to each other with the
     * less than operator
     */
    template <typename PointerComparableOne, typename PointerComparableTwo>
    using AreValueComparablePointerTypes = std::enable_if_t<std::is_convertible<
        decltype(*std::declval<std::decay_t<PointerComparableOne>>()
                < *std::declval<std::decay_t<PointerComparableTwo>>()),
        bool>::value>;

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

template <typename Type>
std::add_const_t<Type>& as_const(Type& instance) noexcept {
    return instance;
}

template <typename Type>
std::decay_t<Type> decay_copy(Type&& instance) {
    return std::forward<Type>(instance);
}

template <template <typename> class Base, typename Derived>
Derived& Crtp<Base<Derived>>::instance() {
    return static_cast<Derived&>(*this);
}

template <template <typename> class Base, typename Derived>
const Derived& Crtp<Base<Derived>>::instance() const {
    return static_cast<const Derived&>(*this);
}

class LessPtr {
public:

    /**
     * This overload contains the case when the first type of object can be
     * compared when it is on the left hand side of the comparison.  This
     * would come into play when the class for the left object has defined the
     * less than operator as a member function
     *
     * std::enable_if has been used to disable this function when the objects
     * are not dereferencible
     */
    template <typename PointerComparableOne, typename PointerComparableTwo,
              utility_detail::AreValueComparablePointerTypes<
                  PointerComparableOne, PointerComparableTwo>* = nullptr>
    constexpr auto operator()(PointerComparableOne&& lhs,
                              PointerComparableTwo&& rhs) const {
        return *std::forward<PointerComparableOne>(lhs) <
            *std::forward<PointerComparableTwo>(rhs);
    }

    /**
     * Tell the C++ standard library template specializations that this
     * comparator is a transparent comparator
     */
    using is_transparent = std::less<void>::is_transparent;
};

template <typename... Types>
template <typename Type>
Type& VariantMonad<Types...>::cast() & {
    // weird if the user does this since reference types are not allowed
    static_assert(!std::is_same<sharp::Find_t<Type, std::tuple<Types...>>,
                                std::tuple<>>::value, "");
    return *reinterpret_cast<Type*>(&this->storage);
}
template <typename... Types>
template <typename Type>
const Type& VariantMonad<Types...>::cast() const & {
    static_assert(!std::is_same<sharp::Find_t<Type, std::tuple<Types...>>,
                                std::tuple<>>::value, "");
    return *reinterpret_cast<const Type*>(&this->storage);
}
template <typename... Types>
template <typename Type>
Type&& VariantMonad<Types...>::cast() && {
    static_assert(!std::is_same<sharp::Find_t<Type, std::tuple<Types...>>,
                                std::tuple<>>::value, "");
    return std::move(*reinterpret_cast<Type*>(&this->storage));
}
template <typename... Types>
template <typename Type>
const Type&& VariantMonad<Types...>::cast() const && {
    static_assert(!std::is_same<sharp::Find_t<Type, std::tuple<Types...>>,
                                std::tuple<>>::value, "");
    return std::move(*reinterpret_cast<const Type*>(&this->storage));
}

} // namespace sharp
