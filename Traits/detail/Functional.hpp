/**
 * @file Functional.hpp
 * @author Aaryaman Sagar
 *
 * This file contains meta functions that can be used to transform type based
 * traits into another type based traits by binding a few arguments to the
 * type just as std::bind does with callables
 */

#pragma once

#include <type_traits>

namespace sharp {

/**
 * @class Bind
 *
 * Binds the trait metafunction to the arguments as provided by the type list
 */
template <template <typename...> class Trait, typename... ToBind>
struct Bind {
    template <typename... Args>
    struct type {
        static constexpr const bool value = Trait<ToBind..., Args...>::value;
    };
};

/**
 * @class Negate
 *
 * Negates the value trait to contain the negation of whatever the value trait
 * would otherwise contain
 */
template <template <typename...> class Trait>
struct Negate {
    template <typename... Args>
    struct type {
        static constexpr const bool value = !Trait<Args...>::value;
    };
};

/**
 * Convenience wrappers around type traits to allow users to fetch the types
 * from a trait conveniently
 */
// template <template <typename...> class Trait, typename... ToBind>
// template <typename... Args>
// using Bind_t = typename Bind<Trait, ToBind...>::template type<Args...>;
// template <template <typename...> class Trait>
// using Negate_t = typename Negate<Trait>::type;

/**
 * Tests for bind
 */
static_assert(Bind<std::is_same, int>::type<int>::value,
        "sharp::Bind tests failed");

/**
 * Tests for bind
 */
static_assert(!Negate<Bind<std::is_same, int>::type>::type<int>::value,
        "sharp::Negate tests failed");


} // namespace sharp
