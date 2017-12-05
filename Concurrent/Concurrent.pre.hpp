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

        // A list of waiters that the current waiter has responsiblity for,
        // this might contain only other readsrs that the current thread has
        // to signal when it wakes up because the current thread is the lead
        // reader or it contains the entire list of waiters that should be
        // transferred to the lock proxy for checking and passing on when the
        // lock is unlocked
        std::vector<std::shared_ptr<Waiter>> waiters;
    };

    /**
     * @class LockProxyWaitableBase
     *
     * A CRTP base class
     * A lock proxy class that contains a list of waiters the lock might be
     * responsible for.  This is useful for conditional critical sections and
     * is an optimization that helps prevent thundering herds, reference the
     * implementation or sketch of the implementation below
     */
    template <typename LockProxy,
              std::enable_if_t<InstantiatedWithValidCv<LockProxy>>* = nullptr>
    class LockProxyWaitableBase
            : public sharp::Crtp<LockProxyWaitableBase<LockProxy>> {
    public:
        template <typename Func>
        void with_waiters(Func&& func) {
            std::forward<Func>(func)(waiters);
        }

        template <typename Waiters, typename Lock>
        void if_leader_transfer_waiters_to(Waiters& waiters, Lock lock) {
            // avoid locking when there is nothing to transfer
            if (is_leader && !waiters.empty()) {
                auto lck = lock();
                static_cast<void>(lck);

                std::copy(std::make_move_iterator(this->waiters.begin()),
                        std::make_move_iterator(this->waiters.end()),
                        std::back_inserter(waiters));
            }
        }

        std::vector<std::shared_ptr<Waiter<Condition, Cv>>> waiters;
        bool is_leader{false};
    };
    template <typename LockProxy,
              std::enable_if_t<InstantiatedWithValidCv<LockProxy>>* = nullptr>
    class LockProxyWaitableBase<InvalidCv> {};

    /**
     * @class ConditionsImpl
     *
     * This class contains most of the implementation of waiting, a little
     * abstracted away from the details of the types of locks the Concurrent
     * instance was instantiated with
     *
     * It allocates one condition variable per condition, this has the benefit
     * of using the standard C++ API to implement waiting and can be used to
     * minimize thundering herds because this gives precise control of the
     * lifetimes of the threads
     *
     * This class protects the condition to condition variable mapping using a
     * mutex since the conditions can be accessed concurrently if and only if
     * the concurrent class is operating on a reader writer lock.  If however
     * the concurrent class was initialized with an exclusive mutex then the
     * condition maps are by default protected by the mutex, so this class
     * does not even contain a mutex for bookkeeping, saving size bloat on
     * instances of the Concurrent class
     *
     * Waiting is done very differently for exclusive lock only mutexes and
     * mutexes that support shared locking.  Both cases however need to
     * minimize thundering herds - this is especially possible if an
     * implementation decides to wake readers on a shared mutex with priority
     * given to writers, the issue gets compounded if readers wake up and all
     * try and put themselves back on the wait queue, this will cause
     * additional contention on the list.  The implementation makes an effort
     * to solve these problems
     *
     * Before unlocking the mutex exclusive lock holders go through the wait
     * list to find a waiter for which the condition is now true - after a
     * write operation, upon finding such a waiter the lock holder transfers
     * the wait list to the woken thread.  This transfer of ownership to the
     * woken thread helps in two ways - it helps minimize thundering herds by
     * avoiding broadcasting and therefore thundering herds by signalling only
     * one thread and helps minimize lock contention when a thread is
     * successfully woken up.  This is done by signalling before unlocking the
     * mutex.  Sane implementations should then give priority to a thread
     * which has been transferred into a mutexes wait queue via wait morphing
     * to help achieve more deterministic ordering (i.e.  avoid other non
     * waiting threads from coming in and snooping away the lock, causing
     * unnecesary context switches).
     *
     * With reads the implementation gets slightly more tricky, because now
     * the implementation has to deal with another source for possible
     * thundering heards and lifetime issues, read threads that have not been
     * signalled can wake up at any time when other readers are active, see
     * that their condition is true and proceed to drop the condition variable
     * they were waiting for, now when threads signal this waiter they might
     * be accessing destroyed data.  To deal with this the implementation
     * abstracts the wait state away in a shared_ptr<> so all threads with
     * references to the wait state access valid state
     *
     * Read threads don't iterate through the wait queue when they wake up
     * because they don't have the ability to change the internal state.  So
     * any thread waiting for a state change should always still see the state
     * remain the same as before.  When write threads wake readers they do two
     * things - 1) they iterate through the rest of the wait queue and half
     * signal to the any other readers that might have to wake up (except for
     * the first read thread signalled, that one is signalled completely), by
     * not actually signalling but only modifying the data representing the
     * wake state to show that they can wake up, this helps reduce thundering
     * herd situations and is required because readers can themselves wake up
     * at any time when other readers are active spuriously and proceed to
     * read their internal bookkeeping, this makes it impossible to modify
     * this bookkeeping concurrently in some other read thread without more
     * synchronization . This establishes one reader sa sort of a "leader".
     * 2) they then transfer the rest of the wait queue to the woken reader,
     * the reader then wakes up and if the reader is able to acquire the lock
     * it checks to see if the condition it was waiting on is still valid and
     * if so, it proceeds to go through the wait queue and wake up readers for
     * which the condition is true, eliminating them from the wait queue as
     * it goes.  If some readers are now at a state where the condition is
     * no longer true, the lead reader puts them back in the regular wait
     * queue under a lock in a batch, this is safe because there is only one
     * lead reader, the rest of the readers don't have anything to do with the
     * wait queue, and other readers don't access the wait queue when they go
     * out of scope, neither do they access any shared state if/when they wake
     * up spuriously, this reduces both lock contention and thundering herds
     */
    template <typename Condition, typename Cv>
    class ConditionsImpl {
    public:

        /**
         * This function should always be called under a write lock before the
         * unlock has been made so that an extra mutex can be avoided when
         * notifying threads
         */
        template <typename LockProxy, typename LockTag, typename Lock>
        void notify_chain(LockProxy& proxy, LockTag, Lock lock) {

            auto waiter = std::shared_ptr<Waiter<Condition, Cv>>{};
            auto proxy_waiters = decltype(this->waiters){};

            proxy.with_waiters([this, &](auto& waiters) {
                // first try and notify the threads that this particular
                // thread has the responsibility of waking
                auto woken = this->wake_one(proxy, waiters, WriteLockTag{})

                // if the attempt was successful then append the wait list to
                // the back of the woken waiter, else put it in the
                // proxy_waiters list to be either handed off to the next
                // woken waiter or put back in the global wait queue
                if (woken) {
                    assert(woken.waiters.empty());
                    woken.waiters = std::move(waiters);
                    waiter = std::move(woken);
                } else {
                    proxy_waiters = std::move(waiters);
                }
            });

            // if a waiter was woken up then do nothing more and return,
            if (waiter) { return; }

            // try and wake someone from the central list of waiters, and if
            // that was successful, append the central list to the back of the
            // woken waiter, if not then append the current list to the
            // central list and return
            if (std::is_same<LockTag, WriteLockTag>::value) {
                waiter = this->wake_one(proxy, this->waiters, LockTag{});
                if (waiter) {
                    assert(waiter.waiters.empty());
                    waiter.waiters = std::move(this->waiters);
                    this->waiters = std::move(proxy_waiters);
                } else {
                    if (!proxy_waiters.empty()) {
                        std::copy(
                                std::make_move_iterator(proxy_waiters.begin()),
                                std::make_move_iterator(proxy_waiters.end()),
                                std::back_inserter(this->waiters));
                    }
                }
            }

            // else just transfer the list of waiters back to the central wait
            // queue
            else {
                proxy.if_leader_transfer_waiters_to(this->waiters, lock);
            }
        }

    protected:
        /**
         * This function waits for the given condition using the given proxy
         * lock.  This assumes that the lock is already held when the wait
         * function is called
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
        void wait(C condition, LockProxy& proxy, Mtx& m, Lock lock, LockTag) {
            // this can only be called if at least a read lock is held on the
            // underlying data item of the concurrent data so no extra
            // synchronization needed when accessing the data and passing to a
            // function that accepts it by const reference
            //
            // Just check for the condition if the condition returns true then
            // short circuit and return
            //
            // This is done outside so that constructions of expensive function
            // objects can be avoided and done only once if this fails
            if (condition(*proxy)) {
                return;
            }

            // if the condition is not true then this thread should set itself
            // up to go on the wait queue
            auto waiter = std::make_shared<Waiter<Condition, Cv>>(
                    false, condition);

            // acquire the lock around the internal bookkeeping.  Note that
            // this is a no-op when wait() is called from an exclusive lock,
            // in that case the bookkeeping is already protected by the
            // exclusive lock
            {
                auto lck = lock();
                static_cast<void>(lck);

                // add the node to the list of waiters
                this->waiters.push_back(waiter);
            }

            while (true) {
                // wait for another thread to signal the current thread, if
                // this thread wakes up without any other thread waking it up
                // (through a spurious wakeup) then no further action is
                // needed because it is already safe on the wait queue waiting
                // to be woken up
                while (!waiter->should_wake) {
                    waiter->cv.wait(m);
                }

                // when the thread if woken up check if the condition is true,
                // if it is then return, because the write thread or leader
                // read thread that signalled the current thread must have
                // taken the current waiter off the wait queue
                //
                // if not then go through the rest of the threads that you
                // have ownership for signal them if possible and re-add
                // yourself to the wait queue, because those threads will not
                // be touched by anyone else.  Those are now the current
                // thread's responsibility and either the current thread needs
                // to pass that responsibility on to another thread that it
                // might wake or put all those threads back in the global wait
                // list
                if (condition(*proxy)) {
                    // go through the list of threads that you might have
                    // responsibility for and signal them
                    return;
                } else {
                }
            }
        }

    private:

        template <typename Proxy, typename WaitList, typename LockTag>
        auto /* shared_ptr<Waiter<>> */ wake_one(
                Proxy& proxy, WaitList& waiters, LockTag) {

            // if a read thread called this then don't do anything, leader
            // read threads call this with a WriteLockTag
            if (std::is_same<LockTag, ReadLockTag>::value) {
                return std::shared_ptr<Waiter<Condition, Cv>>{};
            }

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

        std::vector<std::shared_ptr<Waiter>> waiters;
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
            this->Super::notify_chain(proxy, WriteLockTag{});

            // then go to sleep
            this->Super::wait(std::forward<C>(condition), proxy, lck,
                    [] { return int{}; });
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
