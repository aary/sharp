#pragma once

#include <memory>
#include <utility>
#include <exception>

#include <sharp/Future/Future.hpp>
#include <sharp/Future/detail/FutureImpl.hpp>
#include <sharp/Defer/Defer.hpp>

namespace sharp {

template <typename Type>
Future<Type>::Future() {}

template <typename Type>
Future<Type>::Future(const std::shared_ptr<detail::FutureImpl<Type>>& state)
        : shared_state{state} {
    this->shared_state->test_and_set_retrieved_flag();
}

template <typename Type>
Future<Type>::Future(Future&& other) noexcept
        : shared_state{std::move(other.shared_state)} {}

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
    this->shared_state->wait();
    auto deferred = defer([this]() {
        this->shared_state.reset();
    });
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
