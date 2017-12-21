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
     * Assignment operators from instances of type Type, implicit again for
     * the same reason as above
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
     * Returns a reference to the contained object
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
