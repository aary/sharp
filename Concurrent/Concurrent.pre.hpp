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
     * Lock proxies are a convenient place to store thread local information,
     * so store the list of waiters readers are resonsible for in the lock
     * proxies themselves.  Reference the implementation of waiting below
     */
    template <typename LockProxy,
              std::enable_if_t<InstantiatedWithValidCv<LockProxy>>* = nullptr>
    class LockProxyWaitableBase {
    public:
        std::vector<std::shared_ptr<Waiter<Condition, Cv>>> waiters;
    };
    template <typename LockProxy,
              std::enable_if_t<!InstantiatedWithValidCv<LockProxy>>* = nullptr>
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
     * This class protects the waiter list using a mutex since the conditions
     * can be accessed concurrently when the concurrent class is operating on
     * a reader writer lock.  If however the concurrent class was initialized
     * with an exclusive mutex then the condition maps are by default
     * protected by the mutex, so this class does not even contain a mutex for
     * bookkeeping, saving size bloat on instances of the Concurrent class
     *
     * Waiting is done very differently for exclusive lock only mutexes and
     * mutexes that support shared locking.  Both cases however need to
     * minimize thundering herds - this is especially possible if an
     * implementation decides to wake readers on a shared mutex with priority
     * given to writers, the issue gets compounded if readers wake up and all
     * try and put themselves back on the wait queue, this will cause
     * additional contention on the mutex protecting the waiter list.  The
     * implementation makes an effort to solve these problems by designating
     * one reader as the leader, and give the leader the responsibility of
     * waking up other readers when it wakes up
     *
     * Before unlocking the mutex exclusive lock holders go through the wait
     * list to find a waiter for which the condition is now true (after a
     * write operation), upon finding such a waiter the lock holder the lock
     * holder can do one of two things before signalling the waiter, either it
     * just wakes up the waiter if the waiter is a write thread or it goes
     * through the rest of the list and transfers over the waiters that have
     * their condition set to true to the waiter's thread local cache.  This
     * transfer of ownership to the woken thread helps in two ways - it helps
     * minimize thundering herds by avoiding broadcasting and therefore
     * thundering herds by signalling only one thread and helps minimize lock
     * contention when a thread is successfully woken up.  Also for simplicity
     * signalling is done before unlocking, this relies on the fact that most
     * modern implementations of monitors have wait morphing.  Sane
     * implementations should then give priority to a thread which has been
     * transferred into a mutexes wait queue via wait morphing to help achieve
     * more deterministic ordering (i.e.  avoid other non waiting threads from
     * coming in and snooping away the lock, causing unnecesary context
     * switches).
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
     * synchronization. This establishes one reader as sort of a "leader".
     * 2) they then transfer the rest of the wait queue which have it's
     * conditions to true to the woken reader, the reader then wakes up and if
     * the reader is able to acquire the lock it checks to see if the
     * condition it was waiting on is still valid and if so, it proceeds to go
     * through the wait queue and wake up readers for which the condition is
     * true, eliminating them from the wait queue as it goes.  If some readers
     * are now at a state where the condition is no longer true, the lead
     * reader puts them back in the regular wait queue under a lock in a
     * batch, this is safe because there is only one lead reader, the rest of
     * the readers don't have anything to do with the wait queue, and other
     * readers don't access the wait queue when they go out of scope, neither
     * do they access any shared state if/when they wake up spuriously, this
     * reduces both lock contention and thundering herds
     */
    template <typename Condition, typename Cv>
    class ConditionsImpl {
    public:

        /**
         * This function is used by the top level conditions to wake up
         * threads before unlocking their mutex
         */
        /**
         * The reader specialization does not touch the central wait queue.
         * It operates exclusively on it's own local wait queue.  This helps
         * reduce contention with other read threads that are in the wait()
         * call trying to access the central wait queue
         */
        template <typename LockProxy, typename Lock>
        void notify_chain(LockProxy& proxy, ReadLockTag, Lock lock) {
            // do nothing if the waiters list is empty
            if (proxy.waiters.empty()) { return; }

            // try and wake up waiters from the local list in proxy
            auto waiter = wake_one(proxy, proxy.waiters);

            // if successful then transfer the waiter list over to the woken
            // thread, else grab a lock and put the waiters back in the global
            // wait queue
            if (waiter) {
                // the woken thread cannot be a reader because all readers
                // with valid conditions should already have been woken up
                assert(!waiter->is_reader);
                assert(waiter->waiters.empty());
                waiter->waiters = std::move(proxy.waiters);
            } else {
                auto lck = lock();
                static_cast<void>(lck);
                for (auto& ptr : proxy.waiters) {
                    this->waiters.push_back(std::move(ptr));
                }
            }
        }
        template <typename LockProxy, typename Lock>
        void notify_chain(LockProxy& proxy, WriteLockTag, Lock lock) {
            // write locks should only operate on the global wait queue for
            // simplicitly, since access to internal stuff with write locks is
            // synchronized
            assert(proxy.waiters.empty());

            auto waiter = wake_one(proxy, this->waiters);

            // if could not wake anyone then return, this thread cannot do
            // anything else
            if (!waiter) {
                return;
            }

            // if the waiter is a reader then transfer the list of things that
            // also have their condition set to true to the waiter to be
            // responsible for, the waiter is now the lead reader and will
            // wake the others up if their conditions are satisfied by the
            // time it is able to actually grab the lock, and will otherwise
            // put the read threads that are invalid back in the global wait
            // queue
            //
            // Along with just transferring the list of things that the reader
            // is responsible for, this also needs to set the variable
            // incicating that the read thread has been woken up in the
            // transferred threads, this is needed to prevent against race
            // conditions when other readers (not just the lead reader) wake
            // up spuriously.  The lead reader cannot modify state in other
            // readers because they might wake up spuriously
            if (waiter->is_reader) {
                auto others = half_wake_and_link(proxy, this->waiters,
                        WriteLockTag{});
                assert(waiter->waiters.empty());
                waiter->waiters = std::move(others);
            }
        }

        /**
         * Notifies a thread off either the local wait queue or from the
         * global wait queue.  Specifically this does the following
         *
         *  1. Goes through the local wait queue and tries to find someone to
         *     signal, if found someone then hands off the lcoal wait queue to
         *     that waiter and returns
         *  2. if this is a write thread then also looks in the central wait
         *     queue for a waiter to wake up.  And transfers the local wait
         *     queue back to the central wait queue
         *  3. If there are waiters in the proxy that the thread is
         *     responsible for still, then empty those into the central wait
         *     queue, in the case of readers only the leader should have
         *     threads in the local wait queue
         */
        template <typename LockProxy, typename LockTag, typename Lock>
        void notify_chain(LockProxy& proxy, LockTag, Lock lock) {

            // try and wake up waiters from the local list in proxy
            auto waiter = wake_one(proxy, proxy.waiters);

            // if the attempt was successful then append the proxy's wait list
            // to the back of the woken waiter
            if (waiter) {
                assert(waiter.waiters.empty());
                waiter.waiters = std::move(proxy.waiters);
                return;
            }

            // at this point try and wake someone up from the central list if
            // this is a write thread
            if (std::is_same<LockTag, WriteLockTag>::value) {
                auto waiter = wake_one(proxy, this->waiters);

                // if could wake someone up then transfer the central wait
                // list to the back of that thread
                if (waiter) {
                    assert(waiter.waiters.empty());
                    waiter.waiters = std::move(this->waiters);
                    this->waiters = std::move(proxy.waiters);
                    return;
                }
            }

            // else transfer the list of waiters from the proxy to the central
            // wait list under a lock if this is a reader
            if (!proxy.waiters.empty()) {
                auto lck = lock();
                static_cast<void>(lck);

                std::copy(std::make_move_iterator(proxy.waiters.begin()),
                        std::make_move_iterator(proxy.waiters.end()),
                        std::back_inserter(this->waiters));
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
                    std::is_same<LockTag, ReadLockTag>::value, condition);

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
                    // responsibility for and signal them also if this is a
                    // read thread
                    return;
                } else {
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

        /**
         * Half wakes other readers and returns the list of waiters that the
         * lead reader is going to be responsible for
         */
        template <typename Proxy, typename Waiters, typename Tag>
        static auto half_wake_and_link(Proxy& proxy, Waiters& waiters, Tag) {
            static_assert(std::is_same<Tag, WriteLockTag>::value, "");

            // go through the list, half wake the other readers that have
            // their condition true, and otherwise just erase the waiters that
            // have their condition true and put it in the list to return
            auto removed_waiters = Waiters{};
            auto other_waiters = Waiters{};
            auto deferred = sharp::defer([this, &]() {
                this->waiters = std::move(other_waiters);
            });

            for (auto& waiter : this->waiters) {
                if (waiter->condition(*proxy)) {
                    // if the waiter is a reader then half wake it
                    if (waiter->is_reader) {
                        waiter->should_wake = true;
                    }
                    removed_waiters.push_back(std::move(waiter));
                } else {
                    other_waiters.push_back(std::move(waiter));
                }
            }

            return removed_waiters;
        }

        /**
         * Wakes all the read threads in the wait queue passed, here the
         * read thread doing the waking can not modify the should_wake
         * variable, as that should only be touched by a write thread that
         * does the waking.  If the leader read thread were doing the waking
         * then there would be 2 problems - 1) the should_wake variable would
         * not be thread safe to access through many read threads (if say a
         * read thread spuriously wakes up) and 2) there might be ordering
         * problems that lead to a read thread never waking up because it
         * might wake up, see that the should_wake boolean is false, and then
         * go back to sleep (in the middle the update might come in)
         */
        template <typename Proxy, typename Waiters>
        static auto wake_all(Proxy& proxy, Waiters& waiters) {
            auto other_waiters = Waiters{};
            for (auto& waiter : waiters) {
                if (waiter->condition(*proxy)) {
                    waiter->cv.notify_one();
                } else {
                    other_waiters.push_back(std::move(waiter));
                }
            }
            waiters = std::move(other_waiters);
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
