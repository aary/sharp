/**
 * @file UniqueLock.hpp
 * @author Aaryaman Sagar
 *
 * Contains a more useful alternative to std::unique_lock that works with
 * other mutex types.  Generalizes the lock and unlock methods to work with
 * other lock acquiring calls other than just lock()
 */

#pragma once

#include <chrono>
#include <thread>

namespace sharp {

/**
 * Generic locking and unlocking functors
 *
 * DefaultLock and DefaultUnlock call .lock() and .unlock() on the mutexes
 * respectively
 *
 * SharedLock and SharedUnlock call .shared_lock() and .shared_unlock() on the
 * mutex
 */
struct DefaultLock;
struct DefaultUnlock;
struct SharedLock;
struct SharedUnlock;

/**
 * @class UniqueLock
 *
 * A strictly superior drop in replacement for std::unique_lock, this
 * generalizes the interface of the contained mutex and can work with mutexes
 * that have different locking API other than std::mutex
 *
 * This can also be used to generalize the type of lock that is acquired on
 * the mutex, for example if you wanted to use std::unique_lock with
 * std::shared_mutex to shared lock and shared unlock the mutex in an RAII
 * manner, you would have to create another mutex class that wraps around
 * std::shared_mutex and provides lock and unlock methods that call
 * shared_lock and shared_unlock repectively, this mitigates those problems
 * with a more uniform API
 */
template <typename Mutex,
          typename Lock = DefaultLock, typename Unlock = DefaultUnlock>
class UniqueLock {
public:

    /**
     * Customary aliases
     */
    using mutex_type = Mutex;
    using locker = Lock;
    using unlocker = Unlock;

    /**
     * Constructors that mimic std::unique_lock
     */
    /**
     * Default constructor initializes the unique lock with no state and
     * therefore does not own the underlying mutex
     */
    UniqueLock() = default;

    /**
     * The move constructor moves from another unique lock and takes ownership
     * of the other mutex if one was owned by the other mutex
     *
     * After this the other mutex is left in a null state where it does not
     * own any mutex and does not have a mutex
     */
    UniqueLock(UniqueLock&& other) noexcept;

    /**
     * Initializes the unique_lock by owning the passed mutex
     *
     * This calls the Locker functor that was passed to this mutex class on
     * the underlying mutex
     */
    explicit UniqueLock(Mutex& mtx_in);

    /**
     * Similar to std::unique_lock this does not start off owning the mutex
     * and is left in a non owned state, to lock the mutex the user must call
     * lock()
     */
    UniqueLock(Mutex& mtx_in, std::defer_lock_t);

    /**
     * Similar to std::try_to_lock_t, this calls try_lock() on the underlying
     * mutex and owns the mutex if the try_lock() method on the underlying
     * mutex returned true
     */
    UniqueLock(Mutex& mtx_in, std::try_to_lock_t);

    /**
     * This assumes that the thread constructing the unique_lock owned the
     * mutex already and goes into a owned state without calling any lock
     * method on the mutex
     */
    UniqueLock(Mutex& mtx_in, std::adopt_lock_t);

    /**
     * Call try_lock_for() on the underlying mutex with the passed duration,
     * and go into an owning state if the try_lock_for() function returned
     * true, otherwise the unique_lock does not own the mutex
     */
    template <typename Rep, typename Period>
    UniqueLock(Mutex& mtx_in,
               const std::chrono::duration<Rep, Period>& duration);

    /**
     * Call try_lock_until() on the underlying mutex with the passed time
     * point, if the unique_lock was not able to acquire the mutex until the
     * given time point is reached the unique_lock does not own the mutex, if
     * the thread was able to acquire the mutex the unique lock does own the
     * mutex and goes into a ownning state
     */
    template <typename Clock, typename Duration>
    UniqueLock(Mutex& mtx_in,
               const std::chrono::time_point<Clock, Duration>& time);

    /**
     * Constructor allows passing of any type of lock function, note that this
     * functionality does not exist in std::mutex
     *
     * If the AcquireLock returns a boolean when passed a reference to the
     * underlying mutex, it's return value is used to set the value of
     * whether or not the mutex owns the underlying mutex
     */
    template <typename AcquireLock>
    UniqueLock(Mutex& mtx_in, AcquireLock locker);

    /**
     * Destructor calls Unlocker{} on the underlying mutex, note that this can
     * throw exceptions like std::unique_lock
     */
    ~UniqueLock();

    /**
     * Assignment operator assigns the mutex to another ravlue uniquelock
     * if the current lock had owned a mutex, that is released before
     * acquisition
     *
     * The other mutex is left in a null state not owning any mutex
     *
     * Calling mutex() on the other unqiue lock after this operation returns
     * nullptr, calling owns_lock() on the other unique lock after this
     * operation returns false
     */
    UniqueLock& operator=(UniqueLock&& other);

    /**
     * The lock() and unlock() methods try and acquire the mutex by calling
     * locker() on the underlying mutex
     *
     * By default these are initialized with the same locker and unlocker that
     * they were initialized with
     */
    template <typename L = Lock>
    void lock(L locker = L{});
    template <typename U = Unlock>
    void unlock(U unlocker = U{});

    /**
     * Calls the try_lock() method on the underlying mutex, and returns the
     * value of that invocation to the user and sets the value of owns_mutex
     * accordingly
     */
    bool try_lock();

    /**
     * Calls the try_lock_for() method on the underlying mutex and returns the
     * value of the invocation to the user and sets the value of owns_mutex
     * accordingly
     */
    template <typename Rep, typename Period>
    bool try_lock_for(const std::chrono::duration<Rep,Period>& duration);

    /**
     * Calls the try_lock_until() method with the given time point on the
     * underlying mutex and returns the value of the invocation to the user
     * and sets the value of owns_mutex accordingly
     */
    template <typename Clock, typename Duration>
    bool try_lock_until(const std::chrono::time_point<Clock, Duration>& time);

    /**
     * swap() exchanges all the state for the two unique locks including the
     * mutex references and the owning booleans
     */
    void swap(UniqueLock& other) noexcept;

    /**
     * Breaks the association of the associated mutex, if any, and *this
     *
     * No locks are unlocked. If the *this held ownership of the associated
     * mutex prior to the call, the caller is now responsible to unlock the
     * mutex
     */
    mutex_type* release() noexcept;

    /**
     * Returns a pointer to the underlying mutex without breaking the
     * association of the mutex and *this
     */
    mutex_type* mutex() const noexcept;

    /**
     * Returns true if the unique lock owns the underlying mutex
     */
    bool owns_lock() const noexcept;

    /**
     * Boolean conversion operator that returns this->owns_lock()
     */
    explicit operator bool() const noexcept;

private:

    void throw_if_no_mutex() const;
    void throw_if_already_owned() const;

    mutex_type* mtx{nullptr};
    bool owns_mutex{false};
};

/**
 * @function make_unique_lock
 *
 * Return a unique_lock type class given a mutex and lock and unlock
 * functions, when passed a std::mutex, this returns a simple std::unique_lock
 *
 * This should be treated as a drop in replacement for constructing
 * std::unique_lock as well as a more specialized version of a RAII lock, and
 * should be used instead of unique_lock
 */
// template <typename Mutex,
          // typename Locker = decltype(lock),
          // typename Unlocker = decltype(unlock)>
// auto make_unique_lock(Mutex& mtx,
                      // Locker locker = Locker{},
                      // Unlocker unlocker = Unlocker{});

/**
 * These overloads do the same thing as they do for the constructor for
 * unique_lock, they either defer locking, try to lock or adopt a lock
 */
// template <typename Mutex,
          // typename Locker = decltype(lock),
          // typename Unlocker = decltype(unlock)>
// auto make_unique_lock(Mutex& mtx, std::defer_lock_t,
                      // Locker locker = Locker{},
                      // Unlocker unlocker = Unlocker{});
// template <typename Mutex,
          // typename Locker = decltype(try_lock),
          // typename Unlocker = decltype(unlock)>
// auto make_unique_lock(Mutex& mtx, std::try_to_lock_t,
                      // Locker locker = Locker{},
                      // Unlocker unlocker = Unlocker{});
// template <typename Mutex,
          // typename Locker = decltype(lock),
          // typename Unlocker = decltype(unlock)>
// auto make_unique_lock(Mutex& mtx, std::adopt_lock_t,
                      // Locker locker = Locker{},
                      // Unlocker unlocker = Unlocker{});

/**
 * Call either try_lock_for or try_lock_until on the underlying mutex
 */
// template <typename Mutex, typename Rep, typename Period,
          // typename Locker = decltype(try_lock_for),
          // typename Unlocker = decltype(unlock)>
// auto make_unique_lock(Mutex& mtx,
                      // const std::chrono::duration<Rep, Period>& duration,
                      // Locker locker = Locker{}(duration),
                      // Unlocker unlocker = Unlocker{});
// template <typename Mutex, typename Clock, typename Duration,
          // typename Locker = decltype(try_lock_until),
          // typename Unlocker = decltype(unlock)>
// auto make_unique_lock(Mutex& mtx,
                      // const std::chrono::time_point<Clock, Duration>& time,
                      // Locker locker = Locker{}(time),
                      // Unlocker unlocker = Unlocker{});
} // namespace sharp

#include <sharp/Threads/UniqueLock.ipp>
