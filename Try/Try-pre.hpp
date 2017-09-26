/**
 * @file Try-pre.hpp
 * @author Aaryaman Sagar
 *
 * Contains stuff that doesn't really belong in the implementation or in the
 * main documentation header
 */

#pragma once

#include <sharp/Traits/Traits.hpp>

#include <type_traits>

namespace sharp {

/**
 * Forward declaration to allow checking if a type is an instantiation of Try<>
 */
template <typename T>
class Try;

namespace try_detail {

    template <typename One, typename Two,
              typename O = std::decay_t<One>, typename T = std::decay_t<Two>>
    using EnableIfNotSelf = std::enable_if_t<!std::is_same<O, T>::value>;

    template <typename TryType>
    using EnableIfIsTry = std::enable_if_t<
        sharp::IsInstantiationOf_v<TryType, Try>>;

} // namespace try_detail
} // namepsace sharp
