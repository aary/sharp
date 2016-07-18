#pragma once

#include "LockedData.hpp"

#include <iostream>
#include <mutex>
#include <condition_variable>
#include <utility>
#include <type_traits>


namespace sharp {

namespace detail {

/**
 * @class Unqualified
 *
 * A template utility that removes all reference, const and volatile
 * qualifiers from a type
 */
template <typename Type>
struct Unqualified {
    using type = typename std::remove_cv<
        typename std::remove_reference<Type>::type>::type;
};

/**
 * @alias Unqualified_t
 *
 * A templated typedef that returns an expression equivalent to the
 * following
 *
 *      typename Unqualified<Type>::type;
 */
template <typename Type>
using Unqualified_t = typename Unqualified<Type>::type;

/**
 * Tags to determine which locking policy is considered.
 *
 * ReadLockTag inherits from WriteLockTag because if the implementation gets a
 * standard mutex without a lock_shared method then both the const and non
 * const versions of lock on passing both a ReadLockTag and a WriteLockTag to
 * tbe below functions will not get a compile error because one of them will
 * be disabled by SFINAE.
 *
 * So for example given the following case
 *
 *  LockedData<int, std::mutex> int_locked;
 *
 *  // ... in implementation
 *  lock_mutex(int_locked.mtx, ReadLockTag{}); // (1)
 *
 * this will work even though the ReadLockTag{} dispatches the implementation
 * to the function that calls mtx.lock_shared() on the underlying mutex.  This
 * is because the std::enable_if_t disables that call.  However since the
 * ReadLockTag is inherited from WriteLockTag the function call is still
 * suitable for the write lock implementation.
 */
struct WriteLockTag {};
struct ReadLockTag : public WriteLockTag {};

/**
 * Non member lock function that accepts a mutex by reference and locks it
 * based on its locking functions.  Currently only shared locking and
 * exclusive locking are supported.
 *
 * Note that the function is not called lock() on purpose to disambiguate it
 * from the C++ standard library lock()
 */
template <typename Mutex,
          typename = std::enable_if_t<std::is_same<
            decltype(std::declval<Mutex>().lock_shared()), void>::value>>
void lock_mutex(typename std::add_lvalue_reference<Mutex>::type,
                const ReadLockTag& = ReadLockTag{}) {
    std::cout << "read locking the mutex" << std::endl;
}

/*
 * Non member lock function that accepts a mutex by reference and locks it
 * based on its locking functions.  Currently only shared locking and
 * exclusive locking are supported.
 *
 * Note that the function is not called lock on purpose to disambiguate it
 * from the c++ standard library lock()
 *
 * Also note that this function has a SFINAE disabler which will help in
 * selecting the right dispatch when a mutex that does not support
 * lock_shared() is used
 */
template <typename Mutex>
void lock_mutex(typename std::add_lvalue_reference<Mutex>::type,
                const WriteLockTag& = WriteLockTag{}) {
    std::cout << "write locking the mutex" << std::endl;
}

/**
 * Non member unlock function that accepts a mutex by reference and locks it
 * based on its locking functions.  Currently only shared locking and
 * exclusive locking are supported.
 */
template <typename Mutex>
void unlock_mutex(typename std::add_lvalue_reference<Mutex>::type mtx);

} // namespace detail


/**
 * @class UniqueLockedProxy A proxy class for the internal representation of
 *                          the data object in LockedData that automates
 *                          locking and unlocking of the internal mutex in
 *                          write (non const) scenarios.
 *
 * A proxy class that is locked by the non-const locking policy of the given
 * mutex on construction and unlocked on destruction.
 *
 * So for example when the mutex is a shared lock or a reader writer lock then
 * the internal implementation will choose to write lock the object because
 * the LockedData is not const.
 *
 * TODO If and when the operator.() becomes a thing support should be added to
 * make this a proper proxy.
 */
template <typename Type, typename Mutex>
class LockedData<Type, Mutex>::UniqueLockedProxy {};

/**
 * @class ConstUniqueLockedProxy A proxy class for LockedData that automates
 *                               locking and unlocking of the internal mutex.
 *
 * A proxy class that is locked by the const locking policy of the given mutex
 * on construction and unlocked on destruction.
 *
 * So for example when the mutex is a shared lock or a reader writer lock then
 * the internal implementation will choose to read lock the object because
 * the LockedData object is const and therefore no write access will be
 * granted.
 *
 * This should not be used by the implementation when the internal object is
 * not const.
 *
 * TODO If and when the operator.() becomes a thing support should be added to
 * make this a proper proxy.
 */
template <typename Type, typename Mutex>
class LockedData<Type, Mutex>::ConstUniqueLockedProxy {};

} // namespace sharp
