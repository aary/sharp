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
#include <sharp/Defer/Defer.hpp>
#include <sharp/Threads/Threads.hpp>
#include <sharp/Tags/Tags.hpp>

#include <condition_variable>
#include <mutex>
#include <unordered_map>

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
     * @class CvTraits
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
     * Enable if the cv type is a valid condition variable type
     */
    template <typename Cv>
    using EnableIfIsValidCv
        = std::enable_if_t<!std::is_same<Cv, InvalidCv>::value>;
    template <typename Cv>
    using EnableIfIsInvalidCv
        = std::enable_if_t<std::is_same<Cv, InvalidCv>::value>;

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
    template <typename Condition, typename Cv,
              typename = std::enable_if_t<true>>
    class ConditionsImpl {
    public:

        /**
         * This function should always be called under a write lock before the
         * unlock has been made so that an extra mutex can be avoided when
         * notifying threads
         *
         * TODO test which is faster, an extra mutex before broadcasting or no
         * mutex and piggybacking on the existing mutex (as the current
         * implementation is)
         */
        template <typename LockProxy, typename C = Cv,
                  EnableIfIsValidCv<C>* = nullptr>
        auto notify_all(LockProxy& proxy, WriteLockTag) {

            // imeplement two phase signalling, in the first phase all the
            // stale condition variables are removed from the bookkeeping and
            // in the second phase when the Concurrent item lock has been
            // released, loop through the cvs and call signal on them
            auto cvs = std::vector<std::shared_ptr<Cv>>{};

            for (auto i = this->conditions.begin();
                    i != this->conditions.end();) {

                // if the condition evaluates to true then broadcast and erase
                // that entry from the bookkeeping map because it is not
                // required anymore, the threads have their own reference
                // counted pointer to the condition variable being erased
                if (i->first(*proxy)) {
                    cvs.push_back(i->second);
                    i = this->conditions.erase(i);
                } else {
                    ++i;
                }
            }

            // return a deferrable which will signal all the condition
            // variables on destruction
            return sharp::defer([cvs = std::move(cvs)] {
                for (auto& cv : cvs) {
                    cv->notify_all();
                }
            });
        }

        /**
         * Do nothing when notify_all() is called when a read lock is held,
         * because a read should not change any state within the locked object
         */
        template <typename LockProxy, typename C = Cv,
                  EnableIfIsValidCv<C>* = nullptr>
        int notify_all(LockProxy&, ReadLockTag) { return int{}; }

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
        template <typename LockProxy, typename Mtx, typename Lock>
        void wait(Condition condition, LockProxy& proxy, Mtx& m, Lock lock) {

            // this can only be called if at least a read lock is held on the
            // underlying data item of the concurrent data so check for the
            // condition if the condition returns true then short circuit and
            // return
            if (condition(*proxy)) {
                return;
            }

            // possibly acquire a lock on the map of conditions and add an
            // entry to it if one doesnt exist
            auto cv = std::shared_ptr<Cv>{};
            auto iter = this->conditions.begin();
            {
                // acquire the RAII object and then supress the unused
                // variable warning for the case when this is a no-op
                auto lck = lock();
                static_cast<void>(lck);

                // find or insert the cv condition pair into the map
                iter = this->conditions.find(condition);
                if (iter == this->conditions.end()) {
                    iter = this->conditions.emplace(condition,
                            std::make_shared<Cv>()).first;
                }

                // make a copy that is reference counted with respect to this
                // thread
                cv = iter->second;
            }

            // wait in a loop monitor wait
            while (!condition(*proxy)) {
                iter->second->wait(m);
            }
        }

    private:
        std::unordered_map<Condition, std::shared_ptr<Cv>> conditions;
    };

    template <typename Condition, typename Cv>
    class ConditionsImpl<Condition, Cv, EnableIfIsInvalidCv<Cv>> {
    public:
        template <typename... Args>
        int notify_all(Args&&...) const { return int{}; }
        template <typename... Args>
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

    /**
     * A specialization of the conditions bookkeeping that does not maintain a
     * mutex since the lock around the concurrent object will always be in
     * exclusive mode, so accesses to the bookkeeping will always be
     * serialized
     */
    template <typename Concurrent,
              typename Mutex = typename Concurrent::mutex_type,
              typename Cv = typename Concurrent::cv_type,
              typename Condition = typename Concurrent::Condition_t,
              typename = sharp::void_t<>>
    class Conditions : public ConditionsImpl<Condition, Cv> {
    public:

        using Super = ConditionsImpl<Condition, Cv>;

        template <typename LockProxy>
        void wait(Condition condition, LockProxy& proxy, WriteLockTag) {
            // construct a unique lock to wait on it but release it before
            // returning because the outer lock proxy used in user code should
            // be the one that handles the unlocking on destruction or manual
            // unlock
            auto lck = std::unique_lock<Mutex>{proxy.instance_ptr->mtx,
                std::adopt_lock};
            auto deferred = sharp::defer([&]() { lck.release(); });

            this->Super::wait(condition, proxy, lck, [&] { return int{}; });
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
    template <typename Concurrent, typename Mutex, typename Cv,
              typename Condition>
    class Conditions<Concurrent, Mutex, Cv, Condition,
                     EnableIfIsSharedLockable<Mutex>>
            : public ConditionsImpl<Condition, Cv> {
    public:

        using Super = ConditionsImpl<Condition, Cv>;

        template <typename LockProxy>
        void wait(Condition condition, LockProxy& proxy, ReadLockTag) {
            // construct a shared unique lock to wait on but then release the
            // lock before returning to user code
            auto lck = sharp::UniqueLock<Mutex, sharp::SharedLock>{
                proxy.instance_ptr->mtx, std::adopt_lock};
            auto deferred = sharp::defer([&]() { lck.release(); });

            this->Super::wait(condition, proxy, lck, [&]() {
                return sharp::UniqueLock<Mutex>{this->mtx};
            });
        }
        template <typename LockProxy>
        void wait(Condition condition, LockProxy& proxy, WriteLockTag) {
            // construct a exclusive unique lock to wait on but then release the
            // lock before returning to user code
            auto lck = sharp::UniqueLock<Mutex, sharp::DefaultLock>{
                proxy.instance_ptr->mtx, std::adopt_lock};
            auto deferred = sharp::defer([&]() { lck.release(); });

            this->Super::wait(condition, proxy, lck, [&] { return int{}; });
        }

    private:
        Mutex mtx;
    };

} // namespace concurrent_detail
} // namespace sharp
