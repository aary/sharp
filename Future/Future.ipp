#include <memory>
#include <utility>
#include <exception>

#include <sharp/Future/Future.hpp>
#include <sharp/Future/detail/FutureImpl.hpp>

namespace sharp {

template <typename Type>
Future<Type>::Future() {}

template <typename Type>
Future<Type>::Future(Future&& other)
        : shared_state{std::move(other.shared_state)} {}

template <typename Type>
Future<Type>& Future<Type>::operator=(Future&& other) {
    this->shared_state = std::move(other.shared_state);
}

template <typename Type>
Future<Type>::~Future() {}

template <typename Type>
void Future<Type>::wait() {
    this->shared_state->wait();
}

template <typename Type>
bool Future<Type>::valid() const noexcept {
    // if the future contains a reference count to the shared state then the
    // future is valid
    return static_cast<bool>(this->shared_state);
}

template <typename Type>
Type Future<Type>::get() {
    this->shared_state->wait();
    return this->shared_state->get();
}

} // namespace sharp
