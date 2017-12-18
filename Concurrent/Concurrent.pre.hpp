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
    template <typename Mutex>
    struct GetCv {
        using type = InvalidCv;
    };
    template <>
    struct GetCv<std::mutex> {
        using type = std::condition_variable;
    };

    /**
     * A SFINAE trait for enabling a specialization when the class is
     * instantiated with a valid or invalid condition variable type as the
     * third parameter
     */
    template <typename LockProxy>
    constexpr const auto InstantiatedWithValidCv = false;
    template <template <typename...> class LockProxy,
              typename Type, typename Mutex>
    constexpr const auto
    InstantiatedWithValidCv<LockProxy<Type, Mutex, InvalidCv>> = true;

    /**
     * Waiters provide the functionality needed to put them to sleep.  In the
     * case where std::mutex and std::condition_variable are used, the waiter
     * does not have an additional mutex
     */
    template <typename Mutex>
    struct WaiterBase {
        void lock() {
            mtx.lock();
        }
        void unlock() {
            mtx.unlock();
        }
        template <typename Lock>
        void wait(Lock&) {
            auto lck = std::unique_lock<std::mutex>{this->mtx, std::adopt_lock};
            auto deferred = sharp::defer([&]() { lck.release(); });
            while (!this->should_wake) {
                this->cv.wait(this->mtx);
            }
        }
        template <typename F>
        void notify(F f = [](){}) {
            auto lck = std::unique_lock<std::mutex>{this->mtx};
            this->should_wake = true;
            f();
            cv.notify_one();
        }

        bool should_wake{false};
        std::mutex mtx;
        std::condition_variable cv;
    };
    template <>
    struct WaiterBase<std::mutex> {
        void lock() {}
        void unlock() {}
        void wait(std::mutex& mtx) {
            auto lck = std::unique_lock<std::mutex>{mtx, std::adopt_lock};
            auto deferred = sharp::defer([&]() { lck.release(); });
            while (!this->should_wake) {
                this->cv.wait(lck);
            }
        }
        template <typename F>
        void notify(F f = [](){}) {
            this->should_wake = true;
            f();
            cv.notify_one();
        }

        bool should_wake{false};
        std::condition_variable cv;
    };

    /**
     * @class Waiter
     *
     * The waiter class inherits from WaiterBase to provide the sleeping
     * functionality and also stores the condition object itself and is used
     * to identify whether the thread is a reader or not
     */
    template <typename Condition, typename Mutex>
    struct Waiter : public WaiterBase<Mutex> {
        template <typename C>
        Waiter(bool is_reader_in, bool is_leader_in, C&& condition_in)
            : is_reader{is_reader_in}, is_leader{is_leader_in},
              condition{std::forward<C>(condition_in)} {}

        const bool is_reader;
        bool is_leader;
        const Condition condition;
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
    template <typename Mutex, typename Cv, typename Condition>
    class ConditionsImpl {
    public:

        template <typename LockProxy, typename LockQueue, typename UnlockQueue,
                  typename LockTag>
        void notify_impl(LockProxy& proxy,
                         LockQueue lock_queue, UnlockQueue unlock_queue,
                         LockTag) {
            // if this is a writer or lead reader then try and wake someone up
            // from the wait queue, lead readers should not be able to wake up
            // a reader from the wait list because they should have already
            // been woken up when the lead reader woke up.  Read threads do
            // not have the ability to change the underlying state
            auto is_reader = std::is_same<LockTag, ReadLockTag>::value;
            if ((is_reader && proxy.is_leader) || (!is_reader)) {
                lock_queue();
                for (auto it = this->waiters.begin();
                     it != this->waiters.end();) {
                    if ((*it)->datum.condition(*proxy)) {
                        assert(!(*it)->datum.is_reader);
                        (*it)->datum.notify([]{});
                        this->waiters.erase(it);
                    }
                }
                unlock_queue();
            }
        }

        template <typename C, typename LockProxy, typename Mtx,
                  typename Lock, typename Unlock,
                  typename LockQueue, typename UnlockQueue,
                  typename LockTag>
        void wait_impl(C condition, LockProxy& proxy,
                       Mtx& mutex,
                       Lock lock, Unlock unlock,
                       LockQueue lock_queue, UnlockQueue unlock_queue,
                       LockTag) {
            // if the condition is satisfied already at this point then return
            if (condition(*proxy)) {
                return;
            }

            // otherwise make a wait node and prepare to sleep
            auto&& waiter = WaiterNode{std::in_place,
                false, std::is_same<LockTag, WriteLockTag>::value, condition};

            while (true) {
                // acquire the lock on the queue and also on the waiter
                // itself, the queue mutex protects against data races to the
                // queue and the waiter mutex is used for fine grained locking
                // on the waiter itself, and to ensure that threads don't need
                // to hold the queue lock in order to signal a waiter, this
                // also prevents against spurious waiter wakeups (and thus
                // prevent lifetime problems with waiter state)
                lock_queue();
                waiter.datum.lock();
                waiter.datum.is_leader = std::is_same<LockTag, WriteLockTag>{};
                this->notify_impl(proxy, []{}, []{}, LockTag{});
                waiters.push_back(&waiter);

                // unlock the mutex and the queue mutex and prepare to sleep,
                // at this point the queue contains the waiter which has not
                // slept yet, but wakeups will not be missed because the
                // waiter is synchronized that way
                unlock();
                unlock_queue();
                waiter.datum.wait(mutex);

                // the order of unlocking the queue mutex and locking the main
                // mutex is very important, this is needed to prevent against
                // deadlocks, because the waking thread will be acquiring the
                // lock in the reverse order as compared to one going to
                // sleep, if the queue mutex is not unlocked here
                proxy.is_leader = waiter.datum.is_leader;
                waiter.datum.unlock();
                unlock_queue();
                lock();

                // there are some things that need doing when threads wake up,
                // like chaining reader wake calls, transferring stale waiters
                // back to the global wait queue to prevent locking on lock
                // release in user code for leader readers and transferring
                // the leader baton to another reader for leader reader and
                // going back to sleep yourself
                auto should_return = condition(*proxy);
                this->after_wake(proxy, waiter, lock_queue, unlock_queue,
                                 should_return, LockTag{});

                // if the condition is true now then return
                if (should_return) {
                    return;
                }
            }
        }

    private:
        /**
         * After waking there are some things to be done
         *
         *      1. if the current thread is a reader, then
         *          1. Chain wakeups to other readers
         *          2. If the reader is the leader and the condition is not
         *             satisfied, then wake up other readers
         *      2. if the current thread is a writer, then it must be a leader
         */
        template <typename Proxy, typename Waiter,
                  typename LockQueue, typename UnlockQueue, typename LockTag>
        void after_wake(Proxy& proxy, Waiter& waiter,
                        LockQueue lock_queue,  UnlockQueue unlock_queue,
                        bool should_return, LockTag) {
            if (std::is_same<LockTag, WriteLockTag>::value) {
                assert(proxy.is_leader);
                return;
            }

            // else go through the wait list and wake up every reader you can
            else if (proxy.is_leader) {
                auto has_transferred_baton = false;
                lock_queue();
                for (auto i = this->waiters.begin();
                     i != this->waiters.end();) {

                    if ((*i)->datum.is_reader && (*i)->datum.condition(*proxy)) {
                        // notify the thread to wake up and change the
                        // variables it is associated with while notifying
                        // under lock
                        (*i)->datum.notify([&] {
                            if (!has_transferred_baton && !should_return) {
                                (*i)->datum.is_leader = true;
                                has_transferred_baton = true;
                                proxy.is_leader = false;
                                waiter.datum.is_leader = false;
                            } else {
                                (*i)->datum.is_leader = false;
                            }
                        });
                        i = this->waiters.erase(i);
                    } else {
                        ++i;
                    }
                }
                unlock_queue();
            }
        }

        using WaiterNode = TransparentNode<Waiter<Condition, Mutex>>;
        TransparentList<Waiter<Condition, Mutex>> waiters;
    };

    /**
     * A specialization for when std::mutex is used, no additional
     * synchronization is needed to put the thread to sleep
     */
    template <typename...> struct WhichType;
    template <typename, typename, typename, bool>
    class ConditionsLockWrap;
    template <typename Mutex, typename Cv, typename Condition>
    class ConditionsLockWrap<Mutex, Cv, Condition, false>
            : public ConditionsImpl<Mutex, Cv, Condition> {
    public:
        template <typename LockProxy>
        void notify(LockProxy& proxy, WriteLockTag) {
            this->notify_impl(proxy, []{}, []{}, WriteLockTag{});
        }

        template <typename C, typename LockProxy>
        void wait(C&& condition, LockProxy& proxy, WriteLockTag) {
            auto& mtx = proxy.instance_ptr->mtx;
            this->wait_impl(std::forward<C>(condition), proxy, mtx,
                    [&] { mtx.unlock(); }, [&] { mtx.lock(); },
                    [&] {}, [&] {},
                    WriteLockTag{});
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
        template <typename LockProxy>
        void notify(LockProxy& proxy, ReadLockTag) {
            this->notify_impl(proxy, []{}, []{}, ReadLockTag{});
        }

        template <typename C, typename LockProxy>
        void wait(C&& condition, LockProxy& proxy, ReadLockTag) {
            auto& mtx = proxy.instance_ptr->mtx;
            this->wait_impl(std::forward<C>(condition), proxy, mtx,
                    [&] { mtx.lock_shared(); }, [&] { mtx.unlock_shared(); },
                    [&] { queue_mtx.lock(); }, [&] { queue_mtx.unlock(); },
                    ReadLockTag{});
        }
    private:
        Mutex queue_mtx;
    };

    /**
     * An base class that just inherits from ConditionsLockWrap to provide the
     * wait() and notify() functions.
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
        void notify(Args&&...) const {}
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
