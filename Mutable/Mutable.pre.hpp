/**
 * @file Mutable.pre.hpp
 * @author Aaryaman Sagar
 *
 * Contains a small base class utility to allow EBO with types, and also work
 * with primitive types
 */

#pragma once

#include <type_traits>

#include <sharp/Traits/Traits.hpp>

namespace sharp {
namespace mutable_detail {

    /**
     * Concepts(ish)
     */
    /**
     * Tell whether a class is empty or not
     */
    template <typename T>
    using EnableIfNotEmpty = std::enable_if_t<!std::is_empty<T>::value>;

    template <typename Type, typename = sharp::void_t<>>
    class MutableBase : public Type {
    public:
        Type& get() const {
            // cast away the constness of an empty base because it has no
            // state that can be used to cause UB, method calls on it are safe
            return const_cast<Type&>(static_cast<const Type&>(*this));
        }
    };

    template <typename Type>
    class MutableBase<Type, EnableIfNotEmpty<Type>> {
    public:
        Type& get() const {
            return instance;
        }

        mutable Type instance;
    };

} // namespace mutable_detail
} // namespace sharp
