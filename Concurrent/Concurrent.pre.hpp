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
    class IsSharedLockable : public std::false_type {};
    template <typename Mutex>
    class IsSharedLockable<Mutex, EnableIfIsSharedLockable<Mutex>>
            : public std::true_type {};

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
     * An invalid cv type, signals that the user does not want the ability to
     * sleep
     */
    class InvalidCv {};

    /**
     * A SFINAE trait for enabling a specialization when the class is
     * instantiated with a valid or invalid condition variable type as the
     * third parameter
     */
    template <typename LockProxy>
    constexpr const auto InstantiatedWithValidCv = false;
    {};
    template <template <typename...> class LockProxy,
              typename Type, typename Mutex>
    constexpr const auto
    InstantiatedWithValidCv<LockProxy<Type, Mutex, InvalidCv>> = true;

    /**
     * @class LockProxyWaitableBase
     *
     * Lock proxies are a convenient place to store thread local information,
     * so store the list of waiters readers are resonsible for in the lock
     * proxies themselves.  Reference the implementation of waiting below
     */
    template <typename LockProxy>
    class LockProxyWaitableBase {
        TransparentList<
    };
    template <typename LockProxy,
              std::enable_if_t<!InstantiatedWithValidCv<LockProxy>>* = nullptr>
    class LockProxyWaitableBase<InvalidCv> {};

    /**
     * @class Waiter
     *
     * An implementation detail of conditional critical sections, reference
     * the implementation sketch of ConditionsImpl for more information
     *
     * This contains the state required to wake it up and put it back to sleep
     */
    template <typename Condition, typename Cv>
    struct Waiter {
        template <typename C>
        Waiter(bool is_reader_in, C&& condition_in)
            : is_reader{is_reader_in}, condition{std::forward<C>(condition_in)}
        {}

        bool should_wake{false};
        bool is_reader;
        Condition condition;
        Cv cv{};
    };

    /**
     * @class ConditionsImpl
     *
     * This class contains most of the implementation of waiting, a little
     * abstracted away from the details of the types of locks the Concurrent
     * instance was instantiated with
     *
     * This class uses the standard C++ API to implement sleeping and waking
     * up as needed.  This has the benefit of being a standard implementation
     * and utilizes properties of standard monitor implementations to make the
     * implementation more efficient.  Each waiter is allocated one condition
     * variable, this allows us to pick and chose which thread to wake up
     *
     * Each waiter is stored on the stack and then in an intrusive list to
     * prevent inefficient allocations with each wait.  When the
     * implementation used is not an exclusive mutex only, the wait list
     * itself is protected with a mutex to prevent data races when readers
     * interact with the methods in this class.  If however the implementation
     * does not allow shared locking then the implementation just uses the
     * mutex itself to sleep
     *
     * Waiting is done slightly differently for exclusive lock only mutexes
     * and mutexes that support shared locking.  Both cases however need to
     * minimize thundering herds - which is especially possible if an
     * implementation decides to wake readers on a shared mutex with priority
     * given to writers.  The readers can all wake up and go back to sleep if
     * a writer comes in and changes the underlying data so the conditions are
     * now false.  Further most lock implementations do not go with a
     * condition variable for sleeping, this can cause problems when used with
     * std::condition_variable_any like spourious wakeups, lifetime
     * problems and added context switches.  The implementation tries to solve
     * these problems by using std::mutex coupled with std::condition_variable
     *
     * When a write lock unlocks the mutex it goes through the wait list to
     * see if there are other readers it can wake up, and if it is able to
     * find such a waiter, it signals that waiter to wake up, the waiter then
     * pulls itself off the wait queue when it wakes up
     *
     * In the case of readers, only one reader is woken up and the rest of the
     * wait list is chained to the reader.  This helps reduce thundering herd
     * situations with reader writer locks and minimizes contention on the
     * additional mutex during unlock time - when a reader might have the
     * responsibility of waking up others.  On waking up the reader will then
     * make sure that it signals everyone it can before going to sleep or
     * returning from the wait function, if it is going back to sleep then it
     * will pass on the baton of being the leader to another reader.  All this
     * assumes wait morphing for efficient signalling
     *
     * In the fast path read threads also do not iterate through the rest of
     * the global wait queue when they wake up, as they do not have the
     * ability to change any global state.  The fast path here being when
     * either the read thread does not have the responsibility for other
     * threads or when another thread is in the state where it can wake up, in
     * which case the read thread transfers the rest of the wait list that it
     * is responsible for over to the woken thread.  In the slow path - where
     * the reader is not able to wake anyone up from the list of waiters it
     * has the responsiblity for, it has to make sure someone else can wake up
     * the threads later on if the condition becomes true, so it needs to
     * acquire the lock on the global wait queue and put the waiters back
     */
    template <typename Condition, typename Cv>
    class ConditionsImpl {
    public:

        /**
         * This function is used by the top level conditions to wake up
         * threads before unlocking their mutex
         */
        template <typename LockProxy, typename LockQueue, typename UnlockQueue,
                  typename LockTag>
        void notify(LockProxy& proxy
                    LockQueue lock_queue, UnlockQueue unlock_queue, LockTag) {
            // first look in the local wait queue and try and wake someone up
            for (auto waiter : proxy.waiters) {}
        }

        /**
         * The wait() function puts the current thread to sleep until the
         * condition is satisfied by the writes done by another thread
         */
        template <typename C, typename LockProxy, typename Mtx,
                  typename Lock, typename Unlock,
                  typename LockQueue, typename UnlockQueue,
                  typename LockTag>
        void wait_impl(C condition, LockProxy& proxy,
                       Mtx& queue_mutex,
                       Lock lock, Unlock unlock,
                       LockQueue lock_queue, UnlockQueue unlock_queue,
                       LockTag) {
            // if the condition is satisfied already at this point then return
            if (condition(*proxy)) {
                return;
            }

            // otherwise make a wait node and prepare to sleep
            auto&& waiter = WaiterNode{false, condition};

            while (true) {
                // acquire the lock on the queue, notify others under a lock,
                // the lock is needed to make sure threads don't miss
                // notifications
                lock_queue();
                this->notify(proxy, LockTag{});
                waiters.push_back(&waiter);

                // unlock the mutex and prepare to sleep
                unlock();
                while (!waiter.datum.should_wake) {
                    waiter.datum.cv.wait(queue_mutex);
                }

                // the order of unlocking the queue mutex and locking the main
                // mutex is very important, this is needed to prevent against
                // deadlocks, because the waking thread will be acquiring the
                // lock in the reverse order as compared to one going to
                // sleep, if the queue mutex is not unlocked here
                unlock_queue();
                lock();

                // there are some things that need doing when threads wake up,
                // like chaining reader wake calls, removing yourself from the
                // wait queue and transferring stale waiters back to the
                // global wait queue to prevent locking on lock release in
                // user code
                this->after_wake(proxy, lock_queue, unlock_queue, LockTag{});

                // if the condition is true now then return
                if (condition(*proxy)) {
                    return;
                }
            }
        }

    private:
        template <typename Proxy, typename Waiters>
        static auto /* shared_ptr<Waiter<>> */ wake_one(
                Proxy& proxy, Waiters& waiters) {

            // iterate throught the given list and wake one waiter if possible
            for (auto iter = waiters.begin(); iter != waiters.end(); ++iter) {
                // if the condition evaluates to true then wake up the thread
                // set the signalled boolean and erase the thread from the
                // list of waiters
                auto waiter = *iter;
                if (waiter->condition(*proxy)) {
                    waiter->should_wake = true;
                    waiter->cv.notify_one();
                    auto deferred = sharp::defer([&] { waiters.erase(iter); });
                    return waiter;
                }
            }

            // return a null pointer indicating that nobody was woken up
            return std::shared_ptr<Waiter<Condition, Cv>>{};
        }

        using WaiterNode = TransparentNode<Waiter<Condition, Cv>>;
        TransparentList<Waiter<Condition, Cv>> waiters;
    };

    /**
     * A specialization for when std::mutex is used, no additional
     * synchronization is needed to put the thread to sleep
     */
    template <typename Condition>
    class ConditionsLockWrap<std::mutex, std::condition_variable, Condition>
            : public ConditionsImpl<Condition, Cv> {
    public:
        using Super = ConditionsImpl<Condition, Cv>;

        template <typename C, typename LockProxy>
        void wait(C&& condition, LockProxy& proxy, WriteLockTag) {
            // construct a unique_lock from the already locked mutex without
            // making it acquire ownership of the mutex
            auto lck = std::unique_lock<Mutex>{proxy.instance_ptr->mtx,
                std::defer_lock};
            auto deferred = sharp::defer([&]() { lck.release(); });
            this->Super::wait_impl(std::forward<C>(condition), proxy, lck);
        }
    };

    /**
     * A specialization of conditions bookkeeping that maintains a mutex since
     * the lock around the concurrent object might be held in read only mode,
     * in which case non const accesses to the internal bookkeeping must be
     * serialized explicitly.  For the write case it inherits form the other
     * ConditionsLockWrap class that offers the wait function abstraction for
     * write only locks
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
     * wait() and notify_chain() functions.
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
        void notify_chain(Args&&...) const {}
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
