#pragma once

#include <memory>
#include <utility>
#include <exception>

#include <sharp/Future/Future.hpp>
#include <sharp/Future/FutureError.hpp>
#include <sharp/Future/Promise.hpp>
#include <sharp/Future/detail/FutureImpl.hpp>
#include <sharp/Defer/Defer.hpp>

namespace sharp {

template <typename Type>
Future<Type>::Future() noexcept {}

template <typename Type>
Future<Type>::Future(const std::shared_ptr<detail::FutureImpl<Type>>& state)
        : shared_state{state} {
    this->shared_state->test_and_set_retrieved_flag();
}

template <typename Type>
Future<Type>::Future(Future&& other) noexcept
        : shared_state{std::move(other.shared_state)} {}

template <typename Type>
class WhichType;

template <typename Type>
Future<Type>::Future(Future<Future<Type>>&& other) : Future{} {

    // check if other has shared state and if it does not then throw an
    // exception
    other.check_shared_state();

    // create a promise for *this and assign the resulting future to *this,
    // creating a shared_ptr to a promise because the add_callback function
    // requires a lambda that is copyable, and this is because it stores the
    // lambda in a std::function object
    auto promise = std::make_shared<sharp::Promise<Type>>();
    *this = promise->get_future();

    // clear the shared state of the other future when this function exits
    auto deferred = defer_guard([&other]() {
        other.shared_state.reset();
    });

    // add a continuation to other that will be fired when the other future
    // has been fulfilled with a Future<Type> object
    other.shared_state->add_callback([promise](auto& shared_state_outer)
            mutable {

        // store an exception in the current future if either the inner future
        // is not ready or if there is an exception instead of a future in the
        // outer future
        if (shared_state_outer.contains_exception()) {
            promise->set_exception(shared_state_outer.get_exception_ptr());
            return;
        }
        if (!shared_state_outer.get_value().valid()) {
            auto exc = FutureError{FutureErrorCode::broken_promise};
            promise->set_exception(std::make_exception_ptr(exc));
            return;
        }

        // when the outer future has been fulfilled add a continuation to the
        // inner future that will fire when the inner future has been
        // completed, this will then fulfill *this with the value returned by
        // calling get() on the resulting future
        assert(shared_state_outer.get_value().shared_state);
        shared_state_outer.get_value().shared_state->add_callback(
                [promise] (auto& shared_state_inner) mutable {
            if (shared_state_inner.contains_exception()) {
                promise->set_exception(shared_state_inner.get_exception_ptr());
            } else {
                promise->set_value(shared_state_inner.get());
            }
        });
    });
}

template <typename Type>
Future<Type>& Future<Type>::operator=(Future&& other) noexcept {
    this->shared_state = std::move(other.shared_state);
    return *this;
}

template <typename Type>
Future<Type>::~Future() {}

template <typename Type>
bool Future<Type>::valid() const noexcept {
    // if the future contains a reference count to the shared state then the
    // future is valid
    return static_cast<bool>(this->shared_state);
}

template <typename Type>
void Future<Type>::wait() const {
    this->check_shared_state();
    this->shared_state->wait();
}

template <typename Type>
Type Future<Type>::get() {

    this->check_shared_state();

    // reset the shared state after a successful get(), which will either
    // throw or return the value.  In both cases reset the value
    auto deferred = defer_guard([this]() {
        this->shared_state.reset();
    });
    this->shared_state->wait();
    return this->shared_state->get();
}

template <typename Type>
bool Future<Type>::is_ready() const noexcept {
    this->check_shared_state();
    return this->shared_state->is_ready();
}

template <typename Type>
void Future<Type>::check_shared_state() const {
    if (!this->valid()) {
        throw FutureError{FutureErrorCode::no_state};
    }
}

} // namespace sharp
