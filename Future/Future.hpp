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

namespace sharp {

/**
 * A forward declaration for the class that represents the shared state of the
 * future.  The definition for this class will be shared by code in Future.cpp
 * as well as in Promise.cpp
 *
 * It contains almost all of the synchronization logic required.
 */
class FutureImpl;

template <typename Type>
class Future {

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

private:

    /**
     * A pointer to the implementation for the future.  This implementation
     * code is shared by both futures and promises and contains all the main
     * functionality for futures
     *
     * Both futures and promises hook into methods in this class for
     * functionality.  Both are thin wrappers around FutureImpl
     */
    std::shared_ptr<FutureImpl> shared_state;
};

} // namespace sharp

