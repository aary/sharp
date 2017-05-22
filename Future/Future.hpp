/**
 * @file Future.hpp
 * @author Aaryaman Sagar
 *
 * This file contains an implementation of futures as described in N3721 here
 * https://isocpp.org/files/papers/N3721.pdf
 *
 * In a nutshell a lot of the features supported here allow for asynchrony,
 * most of the focus has been on the callback features that allow futures to
 * be used in asynchronous settings
 */

#pragma once

#include <memory>
#include <functional>
#include <cstddef>

#include <sharp/Traits/Traits.hpp>
#include <sharp/Future/Executor.hpp>
#include <sharp/Future/detail/Future-pre.hpp>

namespace sharp {

namespace detail {

    /**
     * A forward declaration for the class that represents the shared state of
     * the future.  The definition for this class will be shared by code in
     * Future.cpp as well as in Promise.cpp
     *
     * It contains almost all of the synchronization logic required.
     */
    template <typename Type>
    class FutureImpl;

    /**
     * A CRTP base class that implements continuations, this code will be used
     * both for Future and SharedFuture
     */
    template <typename FutureType>
    class ComposableFuture : public sharp::Crtp<FutureType> {
    public:

        /**
         * The basic implementation of .then(), this returns a future that
         * contains the value that is returned by the closure passed to the
         * function, this does not implement unwrapping of the future into
         * another layer, as that is possibly dependent on the future type
         * that inherits from this
         */
        template <typename Func>
        auto then(Func&& func)
                -> Future<decltype(func(std::declval<FutureType>()))>;
    };

} // namespace detail

/**
 * Forward declaration of the Promise class for friendship
 */
template <typename Type>
class Promise;

/**
 * Forward declaration of SharedFuture for friendship
 */
template <typename Type>
class SharedFuture;

/**
 * @class Future
 *
 * A one-shot asymetrical thread safe queue mechanism, the push end of the
 * queue is represented by a Promise object and the pull end of the queue is
 * represented by a Future object
 */
template <typename Type>
class Future : public detail::ComposableFuture<Future<Type>> {
public:

    /**
     * Traits that I thought might be useful
     */
    using value_type = Type;

    /**
     * Constructor for the future constructs the future object without any
     * shared state, any call to the valid() method will return a false.
     *
     * This therefore means that the future will not have any object
     * constructed in it, if the type was instantiated with a class with a
     * default constructor that has side effects, those will not execute until
     * a value is set into the future with a call to set_value() in a
     * corresponding promise object
     */
    Future() noexcept;

    /**
     * Move constructor move constructs a future from another one.  After a
     * call to the move constructor, other.valid() will return false; meaning
     * that if any member functions other than the destructor, the move
     * constructor or valid were to be called on other, then the behavior
     * would be undefined, in most cases assuming there is no race an
     * std::future_error exception will be thrown though
     *
     * The copy constructor is deleted because futures should not be copied.
     * If the requirement is to copy a future and then share state among many
     * other threads then implementations should use a SharedFuture instead
     */
    Future(Future&&) noexcept;
    Future(const Future&) = delete;

    /**
     * Move assignment operator move assigns a future from another one.  After
     * a call to the move assignment operator for a future object, a call to
     * other.valid() will return false (this does not swap two futures) and
     * a call to this->valid() will return whatever other.valid() would return
     * before the call to the move assignment operator
     *
     * The copy assignment operator is deleted because futures should not be
     * copy assigned, if copy assignment is a requirement then users should
     * use SharedFuture
     */
    Future& operator=(Future&&) noexcept;
    Future& operator=(const Future&) = delete;

    /**
     * The destructor for futures releases a reference count on the shared
     * state, so if there are no promises with a reference count to the shared
     * state then it will be destroyed
     */
    ~Future();

    /**
     * Constructs the current future as a proxy to the inner future of the
     * passed in future.  i.e.  The future passed as an argument is unwrapped
     * into the constructed future
     *
     * The implementation creates another shared state for the proxy future
     * that is linked to the inner future, the inner future and the proxy
     * future do not share the same code, although the inner future forwards
     * any values an exceptions to the proxy future the same way.
     *
     * The proxy future "throws" an exception if either the outer future or
     * the inner future have exceptions set in them
     *
     * The unwrapped future is intrinsically linked to the inner future that
     * would be otherwise returned by a call to get(), since they are
     * essentially the same.  Now there cannot be any races between the future
     * set in the shared state of the outer future and the current constructed
     * future.  Why?  Because that future has been moved into this future via
     * proxying
     *
     * Throws an exception in the following conditions
     *
     *  1. other.valid() is false, meaning that there is no shared state in
     *     the current future
     *  2. the inner future.valid() is false, either when the move constructor
     *     is called or when the inner future is set
     *  3. the outer future contains an exception or the inner future contains
     *     an exception
     */
    Future(Future<Future<Type>>&&);

    /**
     * Waits till the future has state constructed in the shared state, this
     * call blocks until the future has been fulfilled via the associated
     * promise
     */
    void wait() const;

    /**
     * Returns a true if the future contains valid shared state that is
     * shared.  Calls to Promise::get_future() return futures that are valid,
     * calls to all factory functions that produce a future should result in a
     * future that is valid
     *
     * Behavior is undefined if valid is false before the calling of this
     * function, but in most cases if there is no race in the future's
     * internals then this will cause the implementation to throw
     * std::future_error exception alerting the user of undefined access
     */
    bool valid() const noexcept;

    /**
     * The get method waits until the future has a valid result and (depending
     * on which template is used) retrieves it. It effectively calls wait() in
     * order to wait for the result.
     *
     * Behavior is undefined if valid is false before the calling of this
     * function, but in most cases if there is no race in the future's
     * internals then this will cause the implementation to throw
     * std::future_error exception alerting the user of undefined access
     *
     * The value returned by a call to this fucntion returns the shared state
     * as if the value had been returned by a call to std::move(object).
     * Therefore the type futures are templated with are required to be move
     * constructible
     *
     * If an exception was stored in the future object via a call to
     * Promise::set_exception() then that exception will be rethrown as if by
     * a call to std::rethrow_exception()
     */
    Type get();

    /**
     * Returns a true if the future contains a value or an exception, this can
     * be used to preemptively either avoid waiting or avoid scheduling a
     * callback to the future
     *
     * Throws an exception if there is no shared state
     */
    bool is_ready() const noexcept;

    /**
     * Shares the current future and returns a shared version of the same
     * future object, a shared future represents a copyable version of future,
     * where the value is not fetched once and only once from the future but
     * rather copied as many times as needed
     *
     * After calling this, the shared state for the future is left empty
     */
    SharedFuture<Type> share();

    /**
     * Attaches a continuation callback to this future that will execute when
     * the future is finished
     *
     * The callback should accept a single argument which is a Future<Type> by
     * value and should return either another value of any type of a future of
     * any type.  The .then() method will return an object of type
     * Future<OtherType> if the callback returns a bare type that is not an
     * instantiation of a future which will get fulfilled when the callback is
     * executed.  .then() will return the same type as the future returned if
     * the callback returns a future itself.  This is done by unwrapping the
     * future in the return value via the unwrapping constructor
     *
     * The callback is executed either on the current thread or on an executor
     * which can be passed in as an optional second parameter
     */
    template <typename Func,
              typename detail::EnableIfDoesNotReturnFuture<Func, Type>*
                  = nullptr>
    auto then(Func&& func) -> Future<decltype(func(std::move(*this)))>;
    template <typename Func,
              typename detail::EnableIfReturnsFuture<Func, Type>* = nullptr>
    auto then(Func&& func) -> decltype(func(std::move(*this)));

    /**
     * Make friends with the promise class
     */
    template <typename T>
    friend class sharp::Promise;

    /**
     * Make friends with all instantiations of the future class, this is
     * needed because there are a lot of methods here which rely on futures
     * that have been instantiated differently from the current instantiation
     * of the future class.  And it is nice to be able to call private methods
     * on these other instantiations as well
     */
    template <typename T>
    friend class sharp::Future;

    /**
     * Make friends with the ComposableFuture class because that uses private
     * members of this class
     */
    template <typename T>
    friend class sharp::detail::ComposableFuture;

    /**
     * Make friends with the SharedFuture class
     */
    template <typename T>
    friend class sharp::SharedFuture;

private:

    /**
     * Construct the future with the shared state passed in.  This should only
     * be called from the Promise class
     */
    Future(const std::shared_ptr<detail::FutureImpl<Type>>& state);

    /**
     * Check if the shared state exists and if it does not then throw an
     * exception
     */
    void check_shared_state() const;

    /**
     * A pointer to the implementation for the future.  This implementation
     * code is shared by both futures and promises and contains all the main
     * functionality for futures
     *
     * Both futures and promises hook into methods in this class for
     * functionality.  Both are thin wrappers around FutureImpl
     */
    std::shared_ptr<detail::FutureImpl<Type>> shared_state;
};

} // namespace sharp

#include <sharp/Future/Future.ipp>
#include <sharp/Future/FutureError.hpp>
#include <sharp/Future/Promise.hpp>
