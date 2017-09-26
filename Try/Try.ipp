#include <sharp/Try/Try.hpp>

#include <type_traits>
#include <utility>
#include <stdexcept>
#include <cassert>
#include <initializer_list>
#include <memory>

namespace sharp {

template <typename T, typename ExceptionPtr>
Try<T, ExceptionPtr>::Try() {}

template <typename T, typename ExceptionPtr>
Try<T, ExceptionPtr>::~Try() {
    if (this->has_value()) {
        this->template cast<T>().~T();
    } else if (this->has_exception()) {
        this->template cast<ExceptionPtr>().~ExceptionPtr();
    }
}

template <typename T, typename ExceptionPtr>
Try<T, ExceptionPtr>::Try(const Try& other) {
    this->construct_from_try(other);
}

template <typename T, typename ExceptionPtr>
Try<T, ExceptionPtr>::Try(Try&& other) {
    this->construct_from_try(std::move(other));
}

template <typename T, typename ExceptionPtr>
template <typename OtherTry,
          try_detail::EnableIfIsTry<OtherTry>*,
          try_detail::EnableIfNotSelf<Try<T, ExceptionPtr>, OtherTry>*>
Try<T, ExceptionPtr>::Try(OtherTry&& other) {
    this->construct_from_try(std::forward<OtherTry>(other));
}

template <typename T, typename ExceptionPtr>
Try<T, ExceptionPtr>::Try(T&& instance) {
    this->emplace(std::move(instance));
}

template <typename T, typename ExceptionPtr>
Try<T, ExceptionPtr>::Try(const T& instance) {
    this->emplace(instance);
}

template <typename T, typename ExceptionPtr>
template <typename... Args>
Try<T, ExceptionPtr>::Try(std::in_place_t, Args&&... args) {
    this->emplace(std::forward<Args>(args)...);
}

template <typename T, typename ExceptionPtr>
template <typename U, typename... Args>
Try<T, ExceptionPtr>::Try(std::in_place_t, std::initializer_list<U> il, Args&&... args) {
    this->emplace(il, std::forward<Args>(args)...);
}

template <typename T, typename ExceptionPtr>
Try<T, ExceptionPtr>::Try(ExceptionPtr ptr) {
    this->state = State::EXCEPTION;
    new (&this->template cast<ExceptionPtr>()) ExceptionPtr{ptr};
}

template <typename T, typename ExceptionPtr>
Try<T, ExceptionPtr>::Try(std::nullptr_t) : Try{} {}

template <typename T, typename ExceptionPtr>
template <typename... Args>
T& Try<T, ExceptionPtr>::emplace(Args&&... args) {
    this->state = State::VALUE;
    auto ptr = new (&this->template cast<T>()) T{std::forward<Args>(args)...};
    return *ptr;
}

template <typename T, typename ExceptionPtr>
template <typename U, typename... Ts>
T& Try<T, ExceptionPtr>::emplace(std::initializer_list<U> il, Ts&&... args) {
    this->state = State::VALUE;
    auto ptr = new (&this->template cast<T>()) T{il, std::forward<Ts>(args)...};
    return *ptr;
}

template <typename T, typename ExceptionPtr>
bool Try<T, ExceptionPtr>::valid() const noexcept {
    return this->has_value() || this->has_exception();
}

template <typename T, typename ExceptionPtr>
bool Try<T, ExceptionPtr>::is_ready() const noexcept {
    return this->valid();
}

template <typename T, typename ExceptionPtr>
Try<T, ExceptionPtr>::operator bool() const noexcept {
    return this->valid();
}

template <typename T, typename ExceptionPtr>
bool Try<T, ExceptionPtr>::has_value() const noexcept {
    return (this->state == State::VALUE);
}

template <typename T, typename ExceptionPtr>
bool Try<T, ExceptionPtr>::has_exception() const noexcept {
    return (this->state == State::EXCEPTION);
}

template <typename T, typename ExceptionPtr>
T& Try<T, ExceptionPtr>::value() & {
    return Try<T, ExceptionPtr>::value_impl(*this);
}

template <typename T, typename ExceptionPtr>
const T& Try<T, ExceptionPtr>::value() const & {
    return Try<T, ExceptionPtr>::value_impl(*this);
}

template <typename T, typename ExceptionPtr>
T&& Try<T, ExceptionPtr>::value() && {
    return Try<T, ExceptionPtr>::value_impl(std::move(*this));
}

template <typename T, typename ExceptionPtr>
const T&& Try<T, ExceptionPtr>::value() const && {
    return Try<T, ExceptionPtr>::value_impl(std::move(*this));
}

template <typename T, typename ExceptionPtr>
T& Try<T, ExceptionPtr>::get() & {
    return Try<T, ExceptionPtr>::value_impl(*this);
}

template <typename T, typename ExceptionPtr>
const T& Try<T, ExceptionPtr>::get() const & {
    return Try<T, ExceptionPtr>::value_impl(*this);
}

template <typename T, typename ExceptionPtr>
T&& Try<T, ExceptionPtr>::get() && {
    return Try<T, ExceptionPtr>::value_impl(std::move(*this));
}

template <typename T, typename ExceptionPtr>
const T&& Try<T, ExceptionPtr>::get() const && {
    return Try<T, ExceptionPtr>::value_impl(std::move(*this));
}

template <typename T, typename ExceptionPtr>
T& Try<T, ExceptionPtr>::operator*() & {
    return this->template cast<T>();
}

template <typename T, typename ExceptionPtr>
const T& Try<T, ExceptionPtr>::operator*() const & {
    return this->template cast<T>();
}

template <typename T, typename ExceptionPtr>
T&& Try<T, ExceptionPtr>::operator*() && {
    return std::move(this->template cast<T>());
}

template <typename T, typename ExceptionPtr>
const T&& Try<T, ExceptionPtr>::operator*() const && {
    return std::move(this->template cast<T>());
}

template <typename T, typename ExceptionPtr>
T* Try<T, ExceptionPtr>::operator->() {
    return std::addressof(this->template cast<T>());
}

template <typename T, typename ExceptionPtr>
const T* Try<T, ExceptionPtr>::operator->() const {
    return std::addressof(this->template cast<T>());
}

template <typename T, typename ExceptionPtr>
ExceptionPtr Try<T, ExceptionPtr>::exception() const {
    if (!this->has_exception()) {
        throw BadTryAccess{"Try does not contain an exception"};
    }
    return this->template cast<ExceptionPtr>();
}

template <typename T, typename ExceptionPtr>
template <typename Other>
void Try<T, ExceptionPtr>::construct_from_try(Other&& other) {
    this->state = other.state;
    if (this->has_value()) {
        new (&this->template cast<T>())
            T{std::forward<Other>(other).template cast<T>()};
    } else if (this->has_exception()) {
        new (&this->template cast<ExceptionPtr>()) ExceptionPtr{
            std::forward<Other>(other).template cast<ExceptionPtr>()};
    }
}

template <typename T, typename ExceptionPtr>
template <typename This>
decltype(auto) Try<T, ExceptionPtr>::value_impl(This&& this_ref) {
    if (this_ref.has_exception()) {
        std::rethrow_exception(std::forward<This>(this_ref)
                .template cast<ExceptionPtr>());
    }
    if (this_ref.has_value()) {
        return std::forward<This>(this_ref).template cast<T>();
    }
    throw BadTryAccess{"Try is empty"};
}

} // namespace sharp
