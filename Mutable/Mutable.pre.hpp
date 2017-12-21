/**
 * @file Mutable.pre.hpp
 * @author Aaryaman Sagar
 *
 * Contains a small base class utility to allow EBO with types, and also work
 * with primitive types
 */

#pragma once

#include <type_traits>

namespace sharp {
namespace mutable_detail {

    /**
     * Concepts(ish)
     */
    /**
     * Tell whether a type is a class type or not
     */
    template <typename T>
    using EnableIfEmpty = std::enable_if_t<std::is_empty<T>>;
    template <typename T>
    using EnableIfNotEmpty = std::enable_if_t<!std::is_empty<T>>;

    template <typename Type, EnableIfEmpty<Type>* = nullptr>
    class MutableBase : public Type {
    public:
        Type& get() const {
            return const_cast<Type&>(static_cast<const Type&>(*this));
        }
    };

    template <typename Type, EnableIfNotClass<Type>* = nullptr>
    class MutableBase {
    public:
        Type& get() const {
            return instance;
        }

        mutable Type instance;
    };

} // namespace mutable_detail
} // namespace sharp
