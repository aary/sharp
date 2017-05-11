#pragma once

#include <memory>
#include <utility>
#include <exception>

#include <sharp/Future/Future.hpp>
#include <sharp/Future/detail/FutureImpl.hpp>

namespace sharp {

template <typename Type>
Future<Type>::Future() {}

template <typename Type>
Future<Type>::Future(std::shared_ptr<detail::FutureImpl<Type>> shared_state_in)
        : shared_state{std::move(shared_state_in)} {}

template <typename Type>
Future<Type>::Future(Future&& other) noexcept
        : shared_state{std::move(other.shared_state)} {}

template <typename Type>
Future<Type>& Future<Type>::operator=(Future&& other) noexcept {
    this->shared_state = std::move(other.shared_state);
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
    return this->shared_state->get();
}

template <typename Type>
void Future<Type>::check_shared_state() const {
    if (!this->valid()) {
        throw FutureError{FutureErrorCode::no_state};
    }
}

} // namespace sharp
