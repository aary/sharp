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
    explicit UniqueLockedProxyBase(Type& object, Mutex& mtx_in)
            : datum{object}, mtx{mtx_in}, owns_mutex{true} {
        detail::lock_mutex(this->mtx, LockTag{});
    }

    /**
     * Move constructs a locked proxy object from another.  The main reason
     * for this is such that the following construct can exist in C++11 and
     * C++14 programs
     *
     *  auto proxy = locked_object.lock();
     *
     * Although this is not a move in most compiled code the elision will not
     * happen since without this move constructor the code will not compile
     *
     * In C++17 this problem is slightly mitigated because there will be
     * mandatory elision here.
     */
    UniqueLockedProxyBase(UniqueLockedProxyBase&& other)
            : datum{other.datum}, mtx{other.mtx}, owns_mutex{true} {
        other.owns_mutex = false;
    }

    /**
     * Disable the copy constructor and assignment operators because it does
     * not really make much sense to have it enabled here.  The proxy objects
     * are just meant to be handles and are not supposed to have the full
     * functionality of regular objects.
     */
    UniqueLockedProxyBase(const UniqueLockedProxyBase&) = delete;
    UniqueLockedProxyBase& operator=(const UniqueLockedProxyBase&) = delete;
    UniqueLockedProxyBase& operator=(UniqueLockedProxyBase&&) = delete;

    /**
     * Unlocks the inner mutex, this function is written to datum the case
     * when the unlock function throws (which it really shouldn't in correct
     * code.  But in that case an assert in the below code fails.  If asserts
     * are not available on the given machine or are disabled because of some
     * build configeration (like they are in MSVS *I think*)
     */
    ~UniqueLockedProxyBase() {
        try {
            if (this->owns_mutex) {
                unlock_mutex(this->mtx);
            }
        } catch (...) {
            std::terminate();
        }
    }

    /**
     * returns a pointer to the type of the object stored under the hood, this
     * datum should be protected and it's implementation should not be
     * exposed at all
     */
    Type* operator->() {
        return &this->datum;
    }

    /**
     * returns a reference to the internal datum that this proxy is
     * responsible for locking
     */
    Type& operator*() {
        return this->datum;
    }

    /**
     * The datum to the type stored in the LockedData object and the mutex
     * that is used to lock the datum.
     *
     * Note that the type of datum (i.e. Type) might be const qualified in
     * the case of ConstUniqueLockedProxy
     */
    Type& datum;
    Mutex& mtx;

    /**
     * Whether the object owns the mutex or not.  In the case where the object
     * is going to be moved into another then only the object that has been
     * move constructed is going to release the mutex.
     */
    bool owns_mutex;
};

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
class LockedData<Type, Mutex>::UniqueLockedProxy :
    public detail::UniqueLockedProxyBase<Type, Mutex, detail::WriteLockTag> {
public:

    /**
     * The default construct does nothing else other than call the base class
     * constructor
     */
    UniqueLockedProxy(Type& object, Mutex& mtx)
        : detail::UniqueLockedProxyBase<Type, Mutex, detail::WriteLockTag>(
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
    public detail::UniqueLockedProxyBase<const Type, Mutex,
        detail::ReadLockTag> {
public:

    /**
     * The default construct does nothing else other than call the base class
     * constructor
     */
    ConstUniqueLockedProxy(const Type& object, Mutex& mtx)
        : detail::UniqueLockedProxyBase<const Type, Mutex, detail::ReadLockTag>
            (object, mtx) {}
};

template <typename Type, typename Mutex>
template <typename Func>
decltype(auto) LockedData<Type, Mutex>::execute_atomic(Func func) {

    // acquire the locked exclusively by constructing an object of type
    // UniqueLockedProxy
    auto locker = UniqueLockedProxy{this->datum, this->mtx};

    // execute the function on the object and return the result, the lock gets
    // released after the return statement.  Note that in the case of absense
    // of mandatory copy elision the result will still be well formed when
    // used with patterns like read, copy, update.  Since the result if a
    // value will be copied into well.  i.e. the return function will finish
    // and then the lock will be released.
    return func(this->datum);
}

template <typename Type, typename Mutex>
template <typename Func>
decltype(auto) LockedData<Type, Mutex>::execute_atomic(Func func) const {

    // acquire the locked exclusively by constructing an object of type
    // UniqueLockedProxy
    auto locker = ConstUniqueLockedProxy{this->datum, this->mtx};

    // execute the function on the object and return the result, the lock gets
    // released after the return statement.  Note that in the case of absense
    // of mandatory copy elision the result will still be well formed when
    // used with patterns like read, copy, update.  Since the result if a
    // value will be copied into well.  i.e. the return function will finish
    // and then the lock will be released.
    return func(this->datum);
}

template <typename Type, typename Mutex>
typename LockedData<Type, Mutex>::UniqueLockedProxy
LockedData<Type, Mutex>::lock() {
    return LockedData<Type, Mutex>::UniqueLockedProxy {this->datum, this->mtx};
}

template <typename Type, typename Mutex>
typename LockedData<Type, Mutex>::ConstUniqueLockedProxy
LockedData<Type, Mutex>::lock() const {
    return LockedData<Type, Mutex>::ConstUniqueLockedProxy{this->datum,
        this->mtx};
}

// TODO implement
// template <typename Type, typename Mutex>
// LockedData<Type, Mutex>::LockedData(const LockedData<Type, Mutex>& other) {
// }

} // namespace sharp
