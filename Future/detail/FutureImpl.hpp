/**
 * @file FutureImpl.hpp
 * @author Aaryaman Sagar
 *
 * This file contains the main code for the FutureImpl class.  Which is the
 * implementation of the futures code, this contains all the synchronization
 * for when a value is set and stuff like that
 *
 * The Promise and Future classes are simply wrappers around this.  Both hook
 * into methods in this when one wants to set state and when the other waits
 * for the promise to be fulfilled.
 *
 * The Promise class is responsible for creating this shared state but the
 * Future class which in most cases extends beyond the Promise class is
 * responsible for destroying it
 */

#pragma once

#include <exception>
#include <condition_variable>
#include <mutex>
#include <initializer_list>
#include <system_error>

namespace sharp {

namespace detail {

    /**
     * A future impl represents the shared state that a future/promise pair
     * share among them.  This contains all the main code for the futures
     * library
     */
    template <typename Type>
    class FutureImpl {
    public:

        /**
         * The wait function blocks until there is a value or an exception in
         * the shared state, this uses a condition variable to wait internally
         */
        void wait() const;

        /**
         * wait_and_get() first calls wait() to wait for shared state to be
         * ready and then returns the object in the shared state as if by
         * std::move(instance)
         */
        Type get();

        /**
         * Forwarding reference implementations of the set_value function
         */
        template <typename... Args>
        void set_value(Args&&...);
        template <typename U, typename... Args>
        void set_value(std::initializer_list<U> il, Args&&...);

        /**
         * Set an exception in the shared state that will be rethrown on a get
         */
        void set_exception(std::exception_ptr ptr);

        /**
         * Returns true if the future contains either a value or an exception
         */
        bool is_ready() const noexcept;

    private:

        /**
         * The states the future can have, these should be self explanatory
         */
        enum class FutureState : int {
            NotFulfilled,
            ContainsValue,
            ContainsException,
            Fulfilled
        };

        /**
         * Synchronizy locking things
         */
        mutable std::atomic<FutureState> state{FutureState::NotFulfilled};
        mutable std::mutex mtx;
        mutable std::condition_variable cv;

        /**
         * A union containing either an exception_ptr or a value, this should
         * be replaced with a better std::variant once that has been
         * standardized and most compilers support that
         */
        std::aligned_union_t<0, std::exception_ptr, Type> storage;

        /**
         * Return the storage cast to the right type, this should be used for
         * all accesses to the internal object storage
         */
        Type* get_object_storage();

        /**
         * Returns the internal storage cast to an exception_ptr pointer type,
         * this should then be used to construct or rethrow the exception
         */
        std::exception_ptr* get_exception_storage();
        const std::exception_ptr* get_exception_storage() const;

        /**
         * Checking functions that make sure everything is okay in the future
         * and if something is wrong they throw the appropriate exception
         */
        void check_get() const;
        void check_set_value() const;
    };

} // namespace detail

} // namespace sharp

#include <sharp/Future/detail/FutureImpl.ipp>
