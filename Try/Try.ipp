#include <sharp/Try/Try.hpp>

#include <type_traits>
#include <utility>
#include <stdexcept>
#include <cassert>
#include <initializer_list>
#include <memory>

namespace sharp {

template <typename T>
Try<T>::Try() {}

template <typename T>
Try<T>::~Try() {
    if (this->has_value()) {
        this->template cast<T>().~T();
    } else if (this->has_exception()) {
        this->template cast<std::exception_ptr>().~exception_ptr();
    }
}

template <typename T>
Try<T>::Try(const Try& other) {
    this->construct_from_try(other);
}

template <typename T>
Try<T>::Try(Try&& other) {
    this->construct_from_try(std::move(other));
}

template <typename T>
template <typename OtherTry,
          try_detail::EnableIfIsTry<OtherTry>*,
          try_detail::EnableIfNotSelf<Try<T>, OtherTry>*>
Try<T>::Try(OtherTry&& other) {
    this->construct_from_try(std::forward<OtherTry>(other));
}

template <typename T>
Try<T>::Try(T&& instance) {
    this->emplace(std::move(instance));
}

template <typename T>
Try<T>::Try(const T& instance) {
    this->emplace(instance);
}

template <typename T>
template <typename... Args>
Try<T>::Try(std::in_place_t, Args&&... args) {
    this->emplace(std::forward<Args>(args)...);
}

template <typename T>
template <typename U, typename... Args>
Try<T>::Try(std::in_place_t, std::initializer_list<U> il, Args&&... args) {
    this->emplace(il, std::forward<Args>(args)...);
}

template <typename T>
Try<T>::Try(std::exception_ptr ptr) {
    this->state = State::EXCEPTION;
    new (&this->template cast<std::exception_ptr>()) std::exception_ptr{ptr};
}

template <typename T>
Try<T>::Try(std::nullptr_t) : Try{} {}

template <typename T>
template <typename... Args>
T& Try<T>::emplace(Args&&... args) {
    this->state = State::VALUE;
    auto ptr = new (&this->template cast<T>()) T{std::forward<Args>(args)...};
    return *ptr;
}

template <typename T>
template <typename U, typename... Ts>
T& Try<T>::emplace(std::initializer_list<U> il, Ts&&... args) {
    this->state = State::VALUE;
    auto ptr = new (&this->template cast<T>()) T{il, std::forward<Ts>(args)...};
    return *ptr;
}

template <typename T>
bool Try<T>::valid() const noexcept {
    return this->has_value() || this->has_exception();
}

template <typename T>
bool Try<T>::is_ready() const noexcept {
    return this->valid();
}

template <typename T>
Try<T>::operator bool() const noexcept {
    return this->valid();
}

template <typename T>
bool Try<T>::has_value() const noexcept {
    return (this->state == State::VALUE);
}

template <typename T>
bool Try<T>::has_exception() const noexcept {
    return (this->state == State::EXCEPTION);
}

template <typename T>
T& Try<T>::value() & {
    return Try<T>::value_impl(*this);
}

template <typename T>
const T& Try<T>::value() const & {
    return Try<T>::value_impl(*this);
}

template <typename T>
T&& Try<T>::value() && {
    return Try<T>::value_impl(std::move(*this));
}

template <typename T>
const T&& Try<T>::value() const && {
    return Try<T>::value_impl(std::move(*this));
}

template <typename T>
T& Try<T>::get() & {
    return Try<T>::value_impl(*this);
}

template <typename T>
const T& Try<T>::get() const & {
    return Try<T>::value_impl(*this);
}

template <typename T>
T&& Try<T>::get() && {
    return Try<T>::value_impl(std::move(*this));
}

template <typename T>
const T&& Try<T>::get() const && {
    return Try<T>::value_impl(std::move(*this));
}

template <typename T>
T& Try<T>::operator*() & {
    return this->template cast<T>();
}

template <typename T>
const T& Try<T>::operator*() const & {
    return this->template cast<T>();
}

template <typename T>
T&& Try<T>::operator*() && {
    return std::move(this->template cast<T>());
}

template <typename T>
const T&& Try<T>::operator*() const && {
    return std::move(this->template cast<T>());
}

template <typename T>
T* Try<T>::operator->() {
    return std::addressof(this->template cast<T>());
}

template <typename T>
const T* Try<T>::operator->() const {
    return std::addressof(this->template cast<T>());
}

template <typename T>
std::exception_ptr Try<T>::exception() const {
    if (!this->has_exception()) {
        throw BadTryAccess{"Try does not contain an exception"};
    }
    return this->template cast<std::exception_ptr>();
}

template <typename T>
template <typename Other>
void Try<T>::construct_from_try(Other&& other) {
    this->state = other.state;
    if (this->has_value()) {
        new (&this->template cast<T>())
            T{std::forward<Other>(other).template cast<T>()};
    } else if (this->has_exception()) {
        new (&this->template cast<std::exception_ptr>()) std::exception_ptr{
            std::forward<Other>(other).template cast<std::exception_ptr>()};
    }
}

template <typename T>
template <typename This>
decltype(auto) Try<T>::value_impl(This&& this_ref) {
    if (this_ref.has_exception()) {
        std::rethrow_exception(std::forward<This>(this_ref)
                .template cast<std::exception_ptr>());
    }
    if (this_ref.has_value()) {
        return std::forward<This>(this_ref).template cast<T>();
    }
    throw BadTryAccess{"Try is empty"};
}

} // namespace sharp
