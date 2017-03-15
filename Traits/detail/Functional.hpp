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
 * Tests for bind
 */
static_assert(Bind<std::is_same, int>::type<int>::value,
        "sharp::Bind tests failed");

} // namespace sharp
