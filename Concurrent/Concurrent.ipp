#pragma once

#include <sharp/Concurrent/Concurrent.hpp>
#include <sharp/Traits/Traits.hpp>

#include <cassert>
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <utility>
#include <type_traits>

namespace sharp {

namespace concurrent_detail {

    /**
     * Lock methods that are dispatched via the method described above, shared
     * locking is preferrable when the calling methods says so and exclusive
     * locking is the fallback
     */
    template <typename Mutex, EnableIfIsSharedLockable<Mutex>* = nullptr>
    void lock_mutex(Mutex& mtx, ReadLockTag) {
        mtx.lock_shared();
    }
    template <typename Mutex>
    void lock_mutex(Mutex& mtx, WriteLockTag) {
        mtx.lock();
    }

    /**
     * Unlock methods that are dispatched via the method described above,
     * in the documentation for ReadLockTag and WriteLockTag, shared locking
     * is preferred when specified and exclusive locking is the fallback
     */
    template <typename Mutex, EnableIfIsSharedLockable<Mutex>* = nullptr>
    void unlock_mutex(Mutex& mtx, ReadLockTag) {
        mtx.unlock_shared();
    }
    template <typename Mutex>
    void unlock_mutex(Mutex& mtx, WriteLockTag) {
        mtx.unlock();
    }

} // namespace concurrent_detail

/**
 * Implementations for the lock proxy methods
 */
template <typename Type, typename Mutex, typename Cv>
template <typename C, typename LockTag>
Concurrent<Type, Mutex, Cv>::template LockProxy<C, LockTag>::LockProxy(C& c)
        : instance_ptr{&c} {
    concurrent_detail::lock_mutex(this->instance_ptr->mtx, LockTag{});
}

template <typename Type, typename Mutex, typename Cv>
template <typename C, typename LockTag>
Concurrent<Type, Mutex, Cv>::template LockProxy<C, LockTag>::LockProxy(
        LockProxy&& other) : instance_ptr{other.instance_ptr} {
    // make the other null
    other.instance_ptr = nullptr;
}

template <typename Type, typename Mutex, typename Cv>
template <typename C, typename LockTag>
void Concurrent<Type, Mutex, Cv>::template LockProxy<C, LockTag>::unlock()
        noexcept {
    // unlock the mutex and go into a null state
    if (this->instance_ptr) {
        // wake any sleeping threads if their conditions are met
        auto raii = this->instance_ptr->conditions.notify_all(*this, LockTag{});
        static_cast<void>(raii);

        // unlock the mutex, the actual signalling will happen on destruction
        // of the raii object
        concurrent_detail::unlock_mutex(this->instance_ptr->mtx, LockTag{});
        this->instance_ptr = nullptr;
    }
}

template <typename Type, typename Mutex, typename Cv>
template <typename C, typename LockTag>
Concurrent<Type, Mutex, Cv>::template LockProxy<C, LockTag>::~LockProxy() {
    // unlock and go into a null state
    this->unlock();
}

template <typename Type, typename Mutex, typename Cv>
template <typename C, typename Tag>
typename Concurrent<Type, Mutex, Cv>::template LockProxy<C, Tag>::value_type&
Concurrent<Type, Mutex, Cv>::template LockProxy<C, Tag>::operator*() {
    return this->instance_ptr->datum;
}

template <typename Type, typename Mutex, typename Cv>
template <typename C, typename Tag>
typename Concurrent<Type, Mutex, Cv>::template LockProxy<C, Tag>::value_type*
Concurrent<Type, Mutex, Cv>::template LockProxy<C, Tag>::operator->() {
    return std::addressof(this->instance_ptr->datum);
}

template <typename Type, typename Mutex, typename Cv>
template <typename C, typename LockTag>
void Concurrent<Type, Mutex, Cv>::template LockProxy<C, LockTag>::wait(
        Concurrent::Condition_t condition) {
    this->instance_ptr->conditions.wait(condition, *this, LockTag{});
}

/**
 * Implementations for the Concurrent<> methods
 */
template <typename Type, typename Mutex, typename Cv>
template <typename Func>
decltype(auto) Concurrent<Type, Mutex, Cv>::synchronized(Func&& func) {

    // acquire the locked exclusively by constructing an object of type
    // UniqueLockedProxy
    auto lock = this->lock();
    return std::forward<Func>(func)(*lock);
}

template <typename Type, typename Mutex, typename Cv>
template <typename Func>
decltype(auto) Concurrent<Type, Mutex, Cv>::synchronized(Func&& func) const {

    // acquire the locked exclusively by constructing an object of type
    // UniqueLockedProxy
    auto lock = this->lock();
    return std::forward<Func>(func)(*lock);
}

template <typename Type, typename Mutex, typename Cv>
auto Concurrent<Type, Mutex, Cv>::lock() {
    return LockProxy<Concurrent, concurrent_detail::WriteLockTag>{*this};
}

template <typename Type, typename Mutex, typename Cv>
auto Concurrent<Type, Mutex, Cv>::lock() const {
    return LockProxy<const Concurrent, concurrent_detail::ReadLockTag>{*this};
}

/**
 * RAII based constructor decoration implementation
 *
 * This function accepts an action that is used to perform some action before
 * the constructor implementation is ran and clean up afterwards.  The before
 * and cleanup are done through construction and destruction of the Action
 * object.
 */
template <typename Type, typename Mutex, typename Cv>
template <typename Action, typename... Args>
Concurrent<Type, Mutex, Cv>::Concurrent(
        sharp::delegate_constructor::tag_t,
        Action,
        Args&&... args) :
    Concurrent{sharp::implementation::tag, std::forward<Args>(args)...} {}

/**
 * the implementation for the constructors, accepts a forwarding reference to
 * any type of Concurrent object and then forwards it's data into our data
 */
template <typename Type, typename Mutex, typename Cv>
template <typename OtherConcurrent>
Concurrent<Type, Mutex, Cv>::Concurrent(
        sharp::implementation::tag_t, OtherConcurrent&& other)
    : datum{std::forward<OtherConcurrent>(other).datum} {}

/**
 * Copy constructor forwards with an action to the decorated delegate
 * constructor, which in turn forwards to the implementation constructor
 */
template <typename Type, typename Mutex, typename Cv>
Concurrent<Type, Mutex, Cv>::Concurrent(const Concurrent& other)
    : Concurrent{sharp::delegate_constructor::tag, other.lock(), other} {}

/**
 * Same as the copy constructor
 */
template <typename Type, typename Mutex, typename Cv>
Concurrent<Type, Mutex, Cv>::Concurrent(Concurrent&& other)
    : Concurrent{sharp::delegate_constructor::tag, other.lock(),
        std::move(other)} {}

template <typename Type, typename Mutex, typename Cv>
template <typename... Args>
Concurrent<Type, Mutex, Cv>::Concurrent(std::in_place_t, Args&&... args)
    : datum{std::forward<Args>(args)...} {}
template <typename Type, typename Mutex, typename Cv>
template <typename U, typename... Args>
Concurrent<Type, Mutex, Cv>::Concurrent(std::in_place_t,
                                    std::initializer_list<U> il, Args&&... args)
    : datum{il, std::forward<Args>(args)...} {}

/**
 * Implementation for the copy assignment operator.
 *
 * The algorithm used to lock the mutexes is simple.  Lock the one that comes
 * earlier in memory first and then lock the other.
 */
template <typename Type, typename Mutex, typename Cv>
Concurrent<Type, Mutex, Cv>& Concurrent<Type, Mutex, Cv>::operator=(
        const Concurrent& other) {

    // check which one comes first in memory
    if (reinterpret_cast<uintptr_t>(std::addressof(other.mtx)) <
            reinterpret_cast<uintptr_t>(std::addressof(this->mtx))) {

        // lock the two RAII style
        auto other_locked_proxy = other.lock();
        auto this_locked_proxy = this->lock();

        // now do the assignment
        this->datum = other.datum;

    } else /* if this mutex comes first */ {

        // lock in reverse order
        auto this_locked_proxy = this->lock();
        auto other_locked_proxy = other.lock();

        // do the assignment
        this->datum = other.datum;
    }

    return *this;
}

} // namespace sharp
