#pragma once

#include <tuple>
#include <type_traits>
#include <utility>
#include <iterator>

#include <sharp/Traits/detail/Utility.hpp>

namespace sharp {

namespace detail {

    /**
     * Enables if the function type can accept a tuple element along with a
     * integral_constant type for the second argument
     */
    template <typename TupleType, typename Func>
    using EnableIfCanAcceptTwoArguments = std::enable_if_t<std::is_same<
        decltype(std::declval<Func>()(std::get<0>(std::declval<TupleType>()),
                    std::integral_constant<int, 0>{})),
        decltype(std::declval<Func>()(std::get<0>(std::declval<TupleType>()),
                    std::integral_constant<int, 0>{}))>::value>;
    /**
     * Checks if the function can accept one argument only
     */
    template <typename TupleType, typename Func>
    using EnableIfCanAcceptOneArgument = std::enable_if_t<std::is_same<
        decltype(std::declval<Func>()(std::get<0>(std::declval<TupleType>()))),
        decltype(std::declval<Func>()(std::get<0>(std::declval<TupleType>())))>
        ::value>;
    /**
     * Checks if the type represents a compile time range with the std::get<>
     * and std::tuple_size<> traits defined
     */
    template <typename Type>
    using EnableIfCompileTimeRange = std::enable_if_t<
        std::is_same<
            decltype(std::get<0>(std::declval<Type>())),
            decltype(std::get<0>(std::declval<Type>()))>::value
        && std::is_same<std::tuple_size<Type>, std::tuple_size<Type>>::value>;
    /**
     * Checks if the type represents a runtime range
     */
    template <typename Type>
    using EnableIfRuntimeRange = std::enable_if_t<
        std::is_same<
            decltype(std::begin(std::declval<Type>())),
            decltype(std::begin(std::declval<Type>()))>::value
        && std::is_same<
            decltype(std::end(std::declval<Type>())),
            decltype(std::end(std::declval<Type>()))>::value>;
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
     * Implementation of the for_each function for the case where the range is
     * a compile time range
     */
    template <int current, int last>
    struct ForEachTupleImpl {

        template <typename TupleType, typename Func,
                  EnableIfCanAcceptTwoArguments<TupleType, Func>* = nullptr>
        static void impl(TupleType&& tup, Func& func) {

            // call the object at the given index
            func(std::get<current>(std::forward<TupleType>(tup)),
                    std::integral_constant<int, current>{});

            // and then recurse
            ForEachTupleImpl<current + 1, last>::impl(
                    std::forward<TupleType>(tup), func);
        }

        template <typename TupleType, typename Func,
                  EnableIfCanAcceptOneArgument<TupleType, Func>* = nullptr>
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

    /**
     * Implementation of the for_each function in the case where the range is
     * a runtime range
     */
    template <typename Range, typename Func,
              typename std::enable_if_t<std::is_same<
                  decltype(std::declval<Func>()(
                              *std::begin(std::declval<Range>()), 0)),
                  decltype(std::declval<Func>()(
                              *std::begin(std::declval<Range>()), 0))>
                      ::value>* = nullptr>
    void for_each_impl(Range&& range, Func& func) {
        auto first = std::begin(std::forward<Range>(range));
        auto last = std::end(std::forward<Range>(range));
        for (auto index = 0; first != last; ++first, ++index) {
            func(*first, index);
        }
    }
    template <typename Range, typename Func,
              typename std::enable_if_t<std::is_same<
                  decltype(std::declval<Func>()(
                              *std::begin(std::declval<Range>()))),
                  decltype(std::declval<Func>()(
                              *std::begin(std::declval<Range>())))>
                      ::value>* = nullptr>
    void for_each_impl(Range&& range, Func& func) {
        auto wrapped = [&func](auto&& element, int) {
            func(std::forward<decltype(element)>(element));
        };
        detail::for_each_impl(std::forward<Range>(range), wrapped);
    }

    namespace test {

        class NotMoveConstructible {
        public:
            NotMoveConstructible() = default;
            NotMoveConstructible(const NotMoveConstructible&) = default;
            NotMoveConstructible(NotMoveConstructible&&) = delete;
        };

        class MoveConstructible {
        public:
            MoveConstructible() = default;
            MoveConstructible(const MoveConstructible&) = default;
            MoveConstructible(MoveConstructible&&) = default;
        };

    } // namespace test

} // namespace detail

template <typename TupleType, typename Func,
          typename detail::EnableIfCompileTimeRange<TupleType>* = nullptr>
Func for_each(TupleType&& tup, Func func) {

    // call the implementation function and then return the functor, similar
    // to std::for_each
    constexpr auto length = std::tuple_size<std::decay_t<TupleType>>::value;
    detail::ForEachTupleImpl<0, length>
        ::impl(std::forward<TupleType>(tup), func);

    return func;
}

template <typename Range, typename Func,
          typename detail::EnableIfRuntimeRange<Range>* = nullptr>
Func for_each(Range&& tup, Func func) {
    detail::for_each_impl(std::forward<Range>(tup), func);
    return func;
}

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

template <typename Derived>
Derived& Crtp<Derived>::this_instance() {
    return static_cast<Derived&>(*this);
}

template <typename Derived>
const Derived& Crtp<Derived>::this_instance() const {
    return static_cast<const Derived&>(*this);
}

template <typename Type, detail::EnableIfRValueConstructible<Type>* = nullptr>
decltype(auto) move_if_movable(Type&& object) {
    return std::move(object);
}

template <typename Type,
          detail::EnableIfNotRValueConstructible<Type>* = nullptr>
decltype(auto) move_if_movable(const Type& object) {
    return object;
}

static_assert(std::is_same<
        decltype(move_if_movable(
                std::declval<detail::test::MoveConstructible>())),
        detail::test::MoveConstructible&&>::value, "");
static_assert(std::is_same<
        decltype(move_if_movable(
                std::declval<detail::test::NotMoveConstructible>())),
        const detail::test::NotMoveConstructible&>::value, "");


} // namespace sharp
