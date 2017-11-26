/**
 * @file Concurrent.pre.hpp
 * @author Aaryaman Sagar
 *
 * Contains stuff that predeclarations that shouldn't go in either the
 * interface or the implementation file
 *
 * Implementations of the functionality in this module will be found in the
 * implementation file as well.  This just contains declarations enough that
 * yield the modules here usable in the main header file
 */

#pragma once

#include <sharp/Traits/Traits.hpp>
#include <sharp/TransparentList/TransparentList.hpp>
#include <sharp/Defer/Defer.hpp>
#include <sharp/Threads/Threads.hpp>
#include <sharp/Tags/Tags.hpp>
#include <sharp/ForEach/ForEach.hpp>

#include <condition_variable>
#include <mutex>
#include <unordered_map>
#include <cassert>

namespace sharp {
namespace concurrent_detail {

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
     * A value trait using the above SFINAE void_t<> expression
     */
    template <typename Mutex, typename = sharp::void_t<>>
    class IsSharedLockable : public std::integral_constant<bool, false> {};
    template <typename Mutex>
    class IsSharedLockable<Mutex, EnableIfIsSharedLockable<Mutex>>
            : public std::integral_constant<bool, true> {};

    /**
     * Tags to determine which locking policy is considered.
     *
     * ReadLockTag inherits from WriteLockTag because if the implementation
     * gets a standard mutex without a lock_shared method then both the const
     * and non const versions of lock on passing both a ReadLockTag and a
     * WriteLockTag to the below functions will not get a compile error
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
     *
     * Note that sharp::preferred_dispatch is meant for a more general
     * solution to this problem, but in this case these were made before that
     * and they still serve to make the code more readable
     */
    struct WriteLockTag {};
    struct ReadLockTag : public WriteLockTag {};

    /**
     * @class GetCv
     *
     * Contains a mapping of mutex to the cv that goes along with it, if a
     * mapping is not found here then it needs to be manually specified in the
     * declaration of the Concurrent object, else compilation will fail when a
     * user tries to call wait() on a lock proxy
     */
    /**
     * An invalid cv type
     */
    class InvalidCv {};
    template <typename Mutex>
    struct GetCv {
        using type = InvalidCv;
    };
    template <>
    struct GetCv<std::mutex> {
        using type = std::condition_variable;
    };

    /**
     * @class LockProxyWaitableBase
     *
     * A base class that lock proxies should inherit from when the concurrent
     * object is waitable
     *
     * When threads hold a read or write lock the lock proxies hold the waiter
     * lists that they are supposed to signal, this helps model reduce lock
     * contention when there are a lot of threads sleeping.  Since when any
     * one thread notifies another thread to wake up from the centralized
     * waiter list, it also transfers the entire list to that waiter to signal
     * when it unlocks, the waiter than does not have to hold locks on the
     * centralized wait queue when waking up the next thread (if any)
     *
     * A base class for lock proxies that contains all the relevant
     * information for signalling waiters that the current thread has taken
     * responsibility for
     *
     * Read locks should never hold a lock on the centralized wait queue, they
     * should only signal and wake up threads off the internal hand-off queue.
     * If there were no threads to be woken up after a reader, then there can
     * never be any threads that can be woken up after the readers are done,
     * since the readers only read and never modify the shared state
     */
    class LockProxyWaitableBase {};

    /**
     * @class ConditionsImpl
     *
     * This class offers the bookkeeping interface for the conditional
     * critical sections feature of the Concurrent class
     *
     * It allocates one condition variable per condition, so that threads can
     * now reuse conditions and work on the same condition variable
     *
     * This class protects the condition to condition variable mapping using a
     * mutex since the conditions can be accessed concurrently if the
     * concurrent class is operating on a reader writer lock
     *
     * If however the concurrent class was initialized with an exclusive mutex
     * then the condition maps are by default protected by the mutex, so this
     * class does not even contain a mutex for bookkeeping, saving size bloat
     * on instances of the Concurrent class
     */
    template <typename Condition, typename Cv>
    class ConditionsImpl {
    public:

        /**
         * This function should always be called under a write lock before the
         * unlock has been made so that an extra mutex can be avoided when
         * notifying threads
         */
        template <typename LockProxy>
        void notify_all(LockProxy& proxy, WriteLockTag) {
            sharp::for_each(this->waiters, [&](auto waiter, auto, auto iter) {
                // if the condition evaluates to true then wake up the thread
                // set the signalled boolean and erase the thread from the
                // list of waiters
                if (waiter->datum.condition(*proxy)) {
                    waiter->datum.signalled = true;
                    waiter->datum.cv.notify_one();
                    this->waiters.erase(iter);
                }
            });
        }

        /**
         * Do nothing when notify_all() is called when a read lock is held,
         * because a read should not change any state within the locked object
         */
        template <typename LockProxy>
        void notify_all(LockProxy&, ReadLockTag) {}

    protected:
        /**
         * This function waits on the given condition for the passed lock
         * proxy object.  Assumes that the lock proxy is currently valid; uses
         * that to access the data item in the proxy
         *
         * The lock function should construct a RAII lock object that locks
         * the internal condition bookkeeping if required.  This is not
         * required for example if the wait() function is called while holding
         * a write lock on the Concurrent object, since only one thread can
         * call wait() at a time and cannot be called concurrently from many
         * threads.  It is required however when many reader threads can call
         * wait() concurrently
         *
         * A reference to the underlying mutex should also be passed so that
         * wait() can be called on the condition variable for the condition by
         * waiting on that mutex
         */
        template <typename C, typename LockProxy, typename Mtx, typename Lock>
        void wait(C&& condition, LockProxy& proxy, Mtx& m, Lock lock) {

            // this can only be called if at least a read lock is held on
            // the underlying data item of the concurrent data so no extra
            // synchronization needed when accessing the data and passing to a
            // function that accepts it by const reference
            //
            // Just check for the condition if the condition returns true then
            // short circuit and return
            //
            // This is done outside so that constructions of expensive
            // function objects can be avoided and done only once if this
            // fails
            if (condition(*proxy)) {
                return;
            }

            // make a node to be added to the waiters list with the function
            // that should be checked when woken up, at this point cannot
            // afford to avoid the conversion from closure to a
            // heavyweight type erased function object but that should be fine
            // because that overhead will be dwarfed by actually going to sleep
            //
            // This is also required for correctness to prevent against
            // spurious wakeup problems.  If a spurious wakeup happens in
            // the loop below, and this was not outside the loop, the entry in
            // the list would be invalidated since at the next iteration
            // another entry would be added for this and the previous one
            // would be invalidated
            //
            // Note that this is on the stack here because notification for
            // the thread to wake up is happening before the lock is unlocked.
            // If the notifications were happening after the lock was unlocked
            // then there would be a race condition on the signalled variable
            // because the signaller would try and set it to true, and this
            // would be reading it.  The second option of using the condition
            // predicate itself as a measure of whether or not the thread was
            // signalled when unlocking outside the protection of the mutex is
            // incorrect because any other thread might spuriously wakeup and
            // continue when the condition is true, leaving the pointer to the
            // waiter in the waiter list dangling
            auto&& waiter = WaiterNode{std::in_place,
                std::forward<C>(condition)};

            // acquire the lock around the internal bookkeeping.  Note
            // that this is a no-op when wait() is called from an exclusive
            // lock, in that case the bookkeeping is already protected by
            // the exclusive lock
            {
                auto lck = lock();
                static_cast<void>(lck);

                // add the node to the list of waiters
                this->waiters.push_back(&waiter);
            }

            // no need to check the actual condition here, the signalled boolean
            // will only be true when the condition is satisfied
            //
            // set up so that a spurious wakeup will not check the condition
            // because of short circuiting
            while (!waiter.datum.signalled || !waiter.datum.condition(*proxy)) {
                waiter.datum.cv.wait(m);
            }
        }

    private:
        /**
         * An intrusive list of waiters, which are kept on the stack for the
         * blocked threads
         */
        struct Waiter {
            template <typename C>
            explicit Waiter(C&& condition_in)
                    : condition{std::forward<C>(condition_in)} {}

            bool signalled{false};
            Condition condition;
            Cv cv{};
        };
        using WaiterNode = sharp::TransparentNode<Waiter>;
        sharp::TransparentList<Waiter> waiters;
    };

    /**
     * A specialization of the conditions bookkeeping that does not maintain a
     * mutex since the lock around the concurrent object will always be in
     * exclusive mode, so accesses to the bookkeeping will always be
     * serialized
     */
    template <typename Mutex, typename Cv, typename Condition,
              bool IsSharedLockable>
    class ConditionsLockWrap : public ConditionsImpl<Condition, Cv> {
    public:
        using Super = ConditionsImpl<Condition, Cv>;

        template <typename C, typename LockProxy>
        void wait(C&& condition, LockProxy& proxy, WriteLockTag) {
            // construct a unique lock to wait on it but release it before
            // returning because the outer lock proxy used in user code should
            // be the one that handles the unlocking on destruction or manual
            // unlock
            auto lck = std::unique_lock<Mutex>{proxy.instance_ptr->mtx,
                std::adopt_lock};
            auto deferred = sharp::defer([&]() { lck.release(); });

            // notify all waiting threads to wake up if their conditions have
            // evaluated to true before going to sleep
            this->Super::notify_all(proxy, WriteLockTag{});

            // then go to sleep
            this->Super::wait(std::forward<C>(condition), proxy, lck,
                    [] { return int{}; });
        }
    };

    /**
     * A specialization of conditions bookkeeping that maintains a mutex since
     * the lock around the concurrent object might be held in read only mode,
     * in which case non const accesses to the internal bookkeeping must be
     * serialized explicitly
     *
     * Even in this case however if access to the bookkeeping is made when an
     * exclusive lock is held, the locking of the mutex can be eliminated, and
     * this work is done at compile time
     */
    template <typename Mutex, typename Cv, typename Condition>
    class ConditionsLockWrap<Mutex, Cv, Condition, true>
            : public ConditionsLockWrap<Mutex, Cv, Condition, false> {
    public:
        using Super = ConditionsImpl<Condition, Cv>;

        /**
         * The implementation of wait when called under a read lock, this will
         * have to acquire a lock on the bookkeeping struct before adding
         * itself to the wait queue if it does add itself to the wait queue
         */
        template <typename C, typename LockProxy>
        void wait(C&& condition, LockProxy& proxy, ReadLockTag) {
            // construct a shared unique lock to wait on but then release the
            // lock before returning to user code
            auto lck = sharp::UniqueLock<Mutex, sharp::SharedLock>{
                proxy.instance_ptr->mtx, std::adopt_lock};
            auto deferred = sharp::defer([&]() { lck.release(); });

            this->Super::wait(std::forward<C>(condition), proxy, lck, [&]() {
                return sharp::UniqueLock<Mutex>{this->mtx};
            });
        }
    private:
        Mutex mtx;
    };

    /**
     * An base class that just inherits from ConditionsLockWrap to provide the
     * wait() and notify_all() functions.
     *
     * In the case where the CV is an invalid CV, these are no-ops for notify
     * calls but does not allow users to call wait(), i.e. fails at compile
     * time if a user attempts a wait without a condition variable
     */
    template <typename Mutex, typename Cv, typename Condition>
    class Conditions
            : public ConditionsLockWrap<Mutex, Cv, Condition,
                                        IsSharedLockable<Mutex>::value> {};
    template <typename Mutex, typename Condition>
    class Conditions<Mutex, InvalidCv, Condition> {
    public:
        template <typename... Args>
        void notify_all(Args&&...) const {}
        template <typename Cv = InvalidCv, typename... Args>
        void wait(Args&&...) const {
            // this static assert stops compilation at this point with a
            // descriptive error message when waiting on a mutex type that
            // does not have an associated condition variable type.
            //
            // The duck typed nature of templates allows the Concurrent class
            // to be used without issue even when using a lock that does not
            // have an associated condition variable type
            static_assert(!std::is_same<Cv, InvalidCv>::value,
                    "The library was not able to find a condition variable "
                    "that works with the provided mutex type.  Please "
                    "explicitly specify a condition variable type");
        }
    };

} // namespace concurrent_detail
} // namespace sharp
