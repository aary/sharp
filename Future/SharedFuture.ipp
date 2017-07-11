#pragma once

#include <sharp/Traits/Traits.hpp>
#include <sharp/Defer/Defer.hpp>
#include <sharp/Future/SharedFuture.hpp>

#include <type_traits>

namespace sharp {

namespace detail {

    /**
     * Assigns a shared future from the promise
     */
    template <typename Type>
    void move_from_promise(Promise<Type>& promise, SharedFuture<Type>& future) {
        future = promise.get_future().share();
    }

} // namespace detail

template <typename Type>
SharedFuture<Type>::SharedFuture() noexcept {};

template <typename Type>
SharedFuture<Type>::SharedFuture(SharedFuture&& other) noexcept
        : shared_state{std::move(other.shared_state)} {}

template <typename Type>
SharedFuture<Type>::SharedFuture(const SharedFuture& other)
        : shared_state{other.shared_state} {}

template <typename Type>
SharedFuture<Type>::SharedFuture(Future<Type>&& other) noexcept
        : shared_state{std::move(other.shared_state)} {}

template <typename Type>
SharedFuture<Type>::SharedFuture(Future<SharedFuture<Type>>&& other) {

    other.check_shared_state();

    // make a promise future pair, and then assign the shared state from the
    // future to the shared state of the current shared future
    auto promise = sharp::Promise<Type>{};
    this->shared_state = promise.get_future().shared_state;

    // release the shared state for the other future
    auto deferred = defer_guard([&other]() {
        other.shared_state.reset();
    });

    // when the future is completed, assign the shared
    other.then([promise = std::move(promise)](auto future) {

        // if the future contains an exception then propagate that to this
        if (future.shared_state->contains_exception()) {
            promise.set_exception(future.shared_state->get_exception_ptr());
            return;
        }

        // get the shared future from the future
        auto shared_future = future.get();

        // if the shared future is not valid then store an exception in this
        if (!future.shared_state->get_value().valid()) {
            auto exc = FutureError{FutureErrorCode::broken_promise};
            promise.set_exception(std::make_exception_ptr(exc));
            return;
        }

        // hook into the shared future and copy the value when that shared
        // future is completed
        shared_future.shared_state->add_callback(
                [promise = std::move(promise)](auto& state) {
            if (state.contains_exception()) {
                promise.set_exception(state.get_exception_ptr());
            } else {
                promise.set_value(state.get_copy());
            }
        });
    });
}

template <typename Type>
const Type& SharedFuture<Type>::get() const {
    this->check_shared_state();
    this->shared_state->wait();
    return this->shared_state->get_copy();
}

template <typename Type>
bool SharedFuture<Type>::valid() const noexcept {
    return static_cast<bool>(this->shared_state);
}

template <typename Type>
void SharedFuture<Type>::wait() const {
    this->check_shared_state();
    this->shared_state->wait();
}

template <typename Type>
bool SharedFuture<Type>::is_ready() const noexcept {
    this->check_shared_state();
    return this->shared_state.is_ready();
}

template <typename Type>
void SharedFuture<Type>::check_shared_state() const {
    if (!this->valid()) {
        throw FutureError{FutureErrorCode::no_state};
    }
}

template <typename Type>
template <typename Func,
          typename detail::EnableIfDoesNotReturnFutureShared<Func, Type>*>
auto SharedFuture<Type>::then(Func&& func)
        -> Future<decltype(func(*this))> {
    return this->detail::ComposableFuture<SharedFuture<Type>>::then(
            std::forward<Func>(func))
        .via(this->get_executor());
}

template <typename Type>
template <typename Func,
          typename detail::EnableIfReturnsFutureShared<Func, Type>*>
auto SharedFuture<Type>::then(Func&& func)
        -> decltype(func(*this)) {
    using T = typename std::decay_t<decltype(func(*this))>::value_type;
    return Future<T>{this->detail::ComposableFuture<SharedFuture<Type>>::then(
            std::forward<Func>(func))}
        .via(this->get_executor());
}

template <typename Type>
SharedFuture<Type> Future<Type>::share() {
    this->check_shared_state();
    return SharedFuture<Type>{std::move(*this)};
}

} // namespace sharp
