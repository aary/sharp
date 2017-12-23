/**
 * @file Mutable.hpp
 * @author Aaryaman Sagar
 *
 * A small expressive utility to get around the fact that mutable is not a
 * part of the type system, with some added benefits
 */

#pragma once

#include <tuple>
#include <utility>
#include <type_traits>

#include <sharp/Mutable/Mutable.pre.hpp>

/**
 * @class Mutable
 *
 * A utility to allow creation of mutable objects, more flexible than the
 * built in mutable keyword and is clearer in a lot of cases, for example
 *
 *      auto mutexes = std::unordered_map<int, std::mutex>{};
 *      // fill map
 *
 *      // read thread, cannot modify the map
 *      void foo(const std::unordered_map<int, std::mutex>& mutexes) {
 *          auto mutex = mutexes.at(1);
 *          mutex.lock(); // error
 *      }
 *
 * This can be easily fixed by making the mutex member mutable, which it
 * should be
 *
 *      auto mutexes = std::unordered_map<int, sharp::Mutable<std::mutex>>{};
 *
 * There are some other cases where this comes in handy, like making a set of
 * mutable instances, which are logically const (with respect to the hashing
 * or comparison values) but can be changed under the hood
 *
 * The mutable class when made a base class also applies the empty base
 * optimization so it can be used to sub in implementations when needed
 * efficiently
 */
template <typename Type>
class Mutable : public MutableBase<Type> {
public:
    static_assert(!std::is_const<Type>::value, "pls");
    static_assert(!std::is_reference<Type>::value, "no references allowed");

    /**
     * Default the constructors
     */
    Mutable() = default;
    Mutable(Mutable&&) = default;
    Mutable(const Mutable&) = default;
    Mutable& operator=(Mutable&&) = default;
    Mutable& operator=(const Mutable&) = default;

    /**
     * Implicit constructors from instances of type Type, made implicit
     * intentionally for convenience.  This allows both regular list
     * initialization in function parameters, etc as well as the nice list
     * initialization syntax, so things like std::in_place can be avoided
     *
     *      auto var = sharp::Mutable<std::vector<int>>{{1, 2, 3, 4}};
     *
     * This puts the initializer {1, 2, 3, 4} as if it were used to initialize
     * the contained std::vector<int>
     */
    Mutable(const Type& instance) : MutableBase<Type>{instance} {}
    Mutable(Type&& instance) : MutableBase<Type>{std::move(instance)} {}

    /**
     * Assignment operators from instances of type Type, can be used
     * for implicit construction like above
     */
    Mutable& operator=(const Type& instance) {
        this->get() = instance;
        return *this;
    }
    Mutable& operator=(Type&& instance) {
        this->get() = std::move(instance);
        return *this;
    }

    /**
     * Returns a reference to the contained object, note that this can be a
     * problem as with all monadic wrappers when used in rvalue mode, because
     * lifetime of a dereferenced value type is not extended when bound to an
     * rvalue reference or a const lvalue reference.  For example
     *
     *      auto&& value = *sharp::Mutable<int>{};
     *
     * Here `value` will be a dangling reference becasue the lifetime of the
     * Mutable object would have ended after the assignment.  This leads to
     * issues, and is even possible with language constructs like the range
     * based for loop
     *
     *      for (auto value : *sharp::Mutable<std::vector<int>>{{{1, 2, 3}}}) {
     *          cout << value << endl;
     *      }
     *
     * Here the loop will be iterating over an expired value.  This is an
     * unfortunate consequence of the semantics of range based for loops.  The
     * above expands to
     *
     *      auto&& __range = *sharp::Mutable<std::vector<int>>{{{1, 2, 3}}};
     *      for (auto __it = __range.begin(); it != __range.end(); ++__it) {
     *          auto value = *it;
     *          cout << value << endl;
     *      }
     *
     * And this has the same problem as the rvalue reference assignment above.
     * The reference is dangling.  Use with care
     */
    Type& get() const & {
        return MutableBase<Type>::get(*this);
    }
    Type&& get() const && {
        return std::move(this->get());
    }

    /**
     * Pointer like functions to make this behave like a pointer
     */
    Type& operator*() const & {
        return this->get();
    }
    Type&& operator*() const && {
        return std::move(this->get());
    }
    Type* operator->() const {
        return std::addressof(this->get());
    }
};
