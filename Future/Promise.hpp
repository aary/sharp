/**
 * @file Promise.hpp
 * @author Aaryaman Sagar
 *
 * This file contains an implementation of the Promise module, it is the
 * counterpart to futures.  Futures represent a result of a computation that
 * is not yet finished.  Internally futures are marked "finished" when their
 * value is set via a promise.  Promises are like factory objects that are
 * used to both produce futures and fulfill them
 */

#pragma once

#include <exception>
#include <memory>

#include <sharp/Future/Future.hpp>

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
class Promise {

    /**
     * Default constructor for a promise, constructs the promise with an empty
     * shared state
     */
    Promise();

    /**
     * Move constructor that move constructs a promise from another promise
     * object, this moves the shared state from the other promise into this
     * promise.
     *
     * After move construction the other promise has no shared state
     *
     * Promise objects are not copy constructible so delete that
     */
    Promise(Promise&& other) noexcept;
    Promise(const Promise&) = delete;

    /**
     * If the shared state is ready, then the promise decrements the reference
     * count for the shared state.
     *
     * If it is not ready, then the promise stores an exception object of type
     * std::future_error with an error condition
     * std::future_errc::broken_promise, makes the shared state ready and
     * releases it
     */
    ~Promise();

    /**
     * Move assignment operator.  This first abandons the current shared state
     * in the promise object and then swaps the shared state with the current
     * state
     *
     * Promise objects are not copy assignable.
     */
    Promise& operator=(Promise&& other) noexcept;
    Promise& operator=(const Promise&) = delete;

    /**
     * A method that gets a future linked with the shared state, this future
     * can then be queried to get the object from the promise
     *
     * Throws an exception if the future has no shared state, in which case
     * the error category is set to no_state.  The other condition where this
     * function throws an exception is where a future has already been
     * retrieved, the error category for the exception is set to
     * future_already_retrieved
     */
    sharp::Future<Type> get_future();

    /**
     * Atomically stores the value in the shared state and makes the state
     * ready.
     *
     * An exception is thrown if there is no shared state or if there is
     * already a value set in the future/promise pair
     *
     * The set_value function that accepts an emplace_construct::tag_t accepts
     * a list of arguments that are forwarded to the constructor of the type
     * in the shared state.  This would mean that the object is constructed in
     * place
     */
    void set_value(const Type& value);
    void set_value(Type&& value);
    template <typename... Args>
    void set_value(sharp::emplace_construct::tag_t, Args&&... args);

    /**
     * Atomically stores an exception in the future associated with this
     * promise and makes the shared state ready
     *
     * An exeption is thrown if there is no shared state or if there is
     * already a value or exception in the shared state.
     */
    void set_exception(std::exception_ptr ptr);
};
