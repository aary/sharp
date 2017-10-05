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

namespace detail {

    /**
     * Concepts(ish)
     */
    /**
     * Enable if a mutex is shared lockable
     */
    template <typename Mutex>
    using EnableIfIsSharedLockable
        = sharp::void_t<decltype(std::declval<Mutex>().lock_shared()),
                        decltype(std::declval<Mutex>().unlock_shared())>;

    /**
     * Tags to determine which locking policy is considered.
     *
     * ReadLockTag inherits from WriteLockTag because if the implementation
     * gets a standard mutex without a lock_shared method then both the const
     * and non const versions of lock on passing both a ReadLockTag and a
     * WriteLockTag to tbe below functions will not get a compile error
     * because one of them will be disabled by SFINAE.
     *
     * So for example given the following case
     *
     *      Concurrent<int, std::mutex> int_locked;
     *
     *      // ... in implementation
     *      lock_mutex(int_locked.mtx, ReadLockTag{}); // (1)
     *
     * this will work even though the ReadLockTag{} dispatches the
     * implementation to the function that calls mtx.lock_shared() on the
     * underlying mutex.  This is because the std::enable_if_t disables that
     * call.  However since the ReadLockTag is inherited from WriteLockTag the
     * function call is still suitable for the write lock implementation.
     */
    struct WriteLockTag {};
    struct ReadLockTag : public WriteLockTag {};

    /**
     * Lock methods that are dispatched via the method described above, shared
     * locking is preferrable when the calling methods says so and exclusive
     * locking is the fallback
     */
    template <typename Mutex, EnableIfIsSharedLockable<Mutex>* = nullptr>
    void lock_mutex(Mutex& mtx, const ReadLockTag& = ReadLockTag{}) {
        mtx.lock_shared();
    }
    template <typename Mutex>
    void lock_mutex(Mutex& mtx, const WriteLockTag& = WriteLockTag{}) {
        mtx.lock();
    }

    /**
     * Unlock methods that are dispatched via the method described above,
     * in the documentation for ReadLockTag and WriteLockTag, shared locking
     * is preferred when specified and exclusive locking is the fallback
     */
    template <typename Mutex, EnableIfIsSharedLockable<Mutex>* = nullptr>
    void unlock_mutex(Mutex& mtx, const ReadLockTag& = ReadLockTag{}) {
        mtx.unlock_shared();
    }
    template <typename Mutex>
    void unlock_mutex(Mutex& mtx, const WriteLockTag& = WriteLockTag{}) {
        mtx.unlock();
    }

    /**
     * A base class for the two proxy classes that will be returned to the
     * user to interact with the underlying data item
     *
     * The type LockTag should correspond to the different tags defined above
     * that are used to enable and disable different locking policies.
     *
     * The common methods shared by the two classes will be the destructor,
     * the data elements, the operator-> and operator*
     */
    template <typename Type, typename Mutex, typename LockTag>
    class UniqueLockedProxyBase {
    public:

        /**
         * Locks the inner mutex with the tag that this class was instantiated
         * with
         */
        UniqueLockedProxyBase(Type& object, Mutex& mtx_in)
                : datum_ptr{std::addressof(object)}, mtx{mtx_in},
                  owns_mutex{true} {
            detail::lock_mutex(this->mtx, LockTag{});
        }

        /**
         * Move constructs a locked proxy object from another.  The main reason
         * for this is such that the following construct can exist in C++11 and
         * C++14 programs
         *
         *      auto proxy = locked_object.lock();
         *
         * Although this is not a move in most compiled code the elision will
         * not happen since without this move constructor the code will not
         * compile
         *
         * Help the compiler and ASAN like tools detect the problem if any by
         * moving the datum into a null state for the moved from proxy
         *
         * In C++17 this problem is slightly mitigated because there will be
         * mandatory elision here.
         */
        UniqueLockedProxyBase(UniqueLockedProxyBase&& other)
                : datum_ptr{other.datum_ptr}, mtx{other.mtx}, owns_mutex{true} {
            other.owns_mutex = false;
            other.datum_ptr = nullptr;
        }

        /**
         * Disable the copy constructor and assignment operators because it
         * does not really make much sense to have it enabled here.  The proxy
         * objects are just meant to be handles and are not supposed to have
         * the full functionality of regular objects.
         */
        UniqueLockedProxyBase(const UniqueLockedProxyBase&) = delete;
        UniqueLockedProxyBase& operator=(const UniqueLockedProxyBase&) = delete;
        UniqueLockedProxyBase& operator=(UniqueLockedProxyBase&&) = delete;

        /**
         * Release the mutex, the object and go into a null state, this helps
         * detect possible bugs, when ran with ASAN a null dereference will
         * definitely be signalled
         *
         * Note that once the lock has been released this lock object is
         * useless, it cannot be used to access the underlying object and it
         * does not provide any lock method
         */
        void unlock() noexcept {
            if (this->owns_mutex) {
                unlock_mutex(this->mtx, LockTag{});
                this->datum_ptr = nullptr;
            }
        }

        ~UniqueLockedProxyBase() {
            this->unlock();
        }

        /**
         * Pointer like methods
         */
        Type* operator->() {
            return this->datum_ptr;
        }
        Type& operator*() {
            return *this->datum_ptr;
        }

        /**
         * Pointer to the data and a reference to the mutex, no need for the
         * mutex to be null so holding a reference to mutex.  Nullability with
         * the datum helps in detecting illegal access-after-unlock scenarios
         */
        Type* datum_ptr;
        Mutex& mtx;

        /**
         * Whether the object owns the mutex or not.  In the case where the
         * object is going to be moved into another then only the object that
         * has been move constructed is going to release the mutex.
         */
        bool owns_mutex;
    };

} // namespace detail


/**
 * A proxy class for the case when the Concurrent data object is not const
 */
template <typename Type, typename Mutex>
class Concurrent<Type, Mutex>::UniqueLockedProxy :
    public detail::UniqueLockedProxyBase<Type, Mutex, detail::WriteLockTag> {
public:
    using detail::UniqueLockedProxyBase<Type, Mutex, detail::WriteLockTag>
        ::UniqueLockedProxyBase;
};

/**
 * A proxy class for the case when the Concurrent data object is const
 */
template <typename Type, typename Mutex>
class Concurrent<Type, Mutex>::ConstUniqueLockedProxy :
    public detail::UniqueLockedProxyBase<const Type, Mutex,
        detail::ReadLockTag> {
public:
    using detail::UniqueLockedProxyBase<const Type, Mutex, detail::ReadLockTag>
        ::UniqueLockedProxyBase;
};

template <typename Type, typename Mutex>
template <typename Func>
auto Concurrent<Type, Mutex>::synchronized(Func&& func)
        -> decltype(std::declval<Func>()(std::declval<Type&>())) {

    // acquire the locked exclusively by constructing an object of type
    // UniqueLockedProxy
    auto lock = this->lock();
    return std::forward<Func>(func)(*lock);
}

template <typename Type, typename Mutex>
template <typename Func>
auto Concurrent<Type, Mutex>::synchronized(Func&& func) const
        -> decltype(std::declval<Func>()(std::declval<Type&>())) {

    // acquire the locked exclusively by constructing an object of type
    // UniqueLockedProxy
    auto lock = this->lock();
    return std::forward<Func>(func)(*lock);
}

template <typename Type, typename Mutex>
typename Concurrent<Type, Mutex>::UniqueLockedProxy
Concurrent<Type, Mutex>::lock() {
    return Concurrent<Type, Mutex>::UniqueLockedProxy{this->datum, this->mtx};
}

template <typename Type, typename Mutex>
typename Concurrent<Type, Mutex>::ConstUniqueLockedProxy
Concurrent<Type, Mutex>::lock() const {
    return Concurrent<Type, Mutex>::ConstUniqueLockedProxy{this->datum,
        this->mtx};
}

/**
 * RAII based constructor decoration implementation
 *
 * This function accepts an action that is used to perform some action before
 * the constructor implementation is ran and clean up afterwards.  The before
 * and cleanup are done through construction and destruction of the Action
 * object.
 */
template <typename Type, typename Mutex>
template <typename Action, typename... Args>
Concurrent<Type, Mutex>::Concurrent(
        sharp::delegate_constructor::tag_t,
        Action,
        Args&&... args) : Concurrent<Type, Mutex>{
    sharp::implementation::tag, std::forward<Args>(args)...}
{}

/**
 * the implementation for the constructors, accepts a forwarding reference to
 * any type of Concurrent object and then forwards it's data into our data
 */
template <typename Type, typename Mutex>
template <typename OtherConcurrent>
Concurrent<Type, Mutex>::Concurrent(
        sharp::implementation::tag_t, OtherConcurrent&& other)
    : datum{std::forward<OtherConcurrent>(other).datum} {}

/**
 * Copy constructor forwards with an action to the decorated delegate
 * constructor, which in turn forwards to the implementation constructor
 */
template <typename Type, typename Mutex>
Concurrent<Type, Mutex>::Concurrent(const Concurrent<Type, Mutex>& other)
    : Concurrent<Type, Mutex>{sharp::delegate_constructor::tag, other.lock(),
        other} {}

/**
 * Same as the copy constructor
 */
template <typename Type, typename Mutex>
Concurrent<Type, Mutex>::Concurrent(Concurrent&& other)
    : Concurrent{sharp::delegate_constructor::tag, other.lock(),
        std::move(other)} {}

template <typename Type, typename Mutex>
template <typename... Args>
Concurrent<Type, Mutex>::Concurrent(std::in_place_t, Args&&... args)
    : datum{std::forward<Args>(args)...} {}
template <typename Type, typename Mutex>
template <typename U, typename... Args>
Concurrent<Type, Mutex>::Concurrent(std::in_place_t,
                                    std::initializer_list<U> il, Args&&... args)
    : datum{il, std::forward<Args>(args)...} {}

/**
 * Implementation for the copy assignment operator.
 *
 * The algorithm used to lock the mutexes is simple.  Lock the one that comes
 * earlier in memory first and then lock the other.
 */
template <typename Type, typename Mutex>
Concurrent<Type, Mutex>& Concurrent<Type, Mutex>::operator=(
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
