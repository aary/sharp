#pragma once

#include "LockedData.hpp"

#include <cassert>
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <utility>
#include <type_traits>


namespace sharp {

namespace detail {

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
 *
 * Also note that this function has a SFINAE disabler which will help in
 * selecting the right dispatch when a mutex that does not support
 * lock_shared() is used
 */
template <typename Mutex,
          typename = std::enable_if_t<std::is_same<
            decltype(std::declval<Mutex>().lock_shared()), void>::value>>
void lock_mutex(Mutex& mtx,
                const ReadLockTag& = ReadLockTag{}) {
    mtx.lock_shared();
}

/*
 * Non member lock function that accepts a mutex by reference and locks it
 * based on its locking functions.  Currently only shared locking and
 * exclusive locking are supported.
 *
 * Note that the function is not called lock on purpose to disambiguate it
 * from the c++ standard library lock()
 */
template <typename Mutex>
void lock_mutex(Mutex& mtx,
                const WriteLockTag& = WriteLockTag{}) {
    mtx.lock();
}

/**
 * Non member unlock function that accepts a mutex by reference and locks it
 * based on its locking functions.  Currently only shared locking and
 * exclusive locking are supported.
 */
template <typename Mutex>
void unlock_mutex(Mutex& mtx) {
    mtx.unlock();
}

} // namespace detail


/**
 * @class UniqueLockedProxyBase A base class for the proxy classes that are
 *                              used to access the underlying object in
 *                              LockedData
 *
 * The type LockTag should correspond to the different tags defined above that
 * are used to enable and disable different locking policies.
 *
 * The common methods shared by the two classes will be the destructor, the
 * data elements, the operator-> and operator* so there will be no extra
 * overhead because of virtual dispatch.  All methods will be made non
 * virtual.
 */
template <typename Type, typename Mutex, typename LockTag>
class UniqueLockedProxyBase {
public:

    /**
     * locks the inner mutex by passing in a ReadLockTag, read the
     * documentation for what happens when the mutex does not support
     * lock_shared()
     */
    explicit UniqueLockedProxyBase(Type& object, Mutex& mtx_in) :
            mtx{mtx_in}, handle{object} {
        detail::lock_mutex(this->mtx, LockTag{});
    }

    /**
     * Unlocks the inner mutex, this function is written to handle the case
     * when the unlock function throws (which it really shouldn't in correct
     * code.  But in that case an assert in the below code fails.  If asserts
     * are not available on the given machine or are disabled because of some
     * build configeration (like they are in MSVS *I think*)
     */
    ~UniqueLockedProxyBase() {
        try {
            detail::unlock_mutex(this->mtx);
        } catch (...) {
            std::terminate();
        }
    }

    /**
     * returns a pointer to the type of the object stored under the hood, this
     * handle should be protected and it's implementation should not be
     * exposed at all
     */
    Type* operator->() {
        return &this->handle;
    }

    /**
     * returns a reference to the internal handle that this proxy is
     * responsible for locking
     */
    Type& operator*() {
        return this->handle;
    }

    /**
     * The handle to the type stored in the LockedData object and the mutex
     * that is used to lock the handle.
     *
     * Note that the type of handle (i.e. Type) might be const qualified in
     * the case of ConstUniqueLockedProxy
     */
    Mutex& mtx;
    Type& handle;
};

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
class LockedData<Type, Mutex>::UniqueLockedProxy :
    public UniqueLockedProxyBase<Type, Mutex, detail::WriteLockTag> {
public:

    /**
     * The default construct does nothing else other than call the base class
     * constructor
     */
    UniqueLockedProxy(Type& object, Mutex& mtx)
        : UniqueLockedProxyBase<Type, Mutex, detail::WriteLockTag>(
                object, mtx) {}
};

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
class LockedData<Type, Mutex>::ConstUniqueLockedProxy :
    public UniqueLockedProxyBase<const Type, Mutex, detail::ReadLockTag> {
public:

    /**
     * The default construct does nothing else other than call the base class
     * constructor
     */
    ConstUniqueLockedProxy(const Type& object, Mutex& mtx)
        : UniqueLockedProxyBase<const Type, Mutex, detail::ReadLockTag>(
                object, mtx) {}
};


} // namespace sharp
