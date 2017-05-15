#include <initializer_list>
#include <exception>
#include <memory>

#include <sharp/Tags/Tags.hpp>
#include <sharp/Future/Promise.hpp>
#include <sharp/Future/FutureError.hpp>
#include <sharp/Future/detail/FutureImpl.hpp>

namespace sharp {

template <typename Type>
Promise<Type>::Promise()
        : shared_state{std::make_shared<detail::FutureImpl<Type>>()} {}

template <typename Type>
Promise<Type>::~Promise() {
    if (this->shared_state) {
        if (!shared_state->is_ready()) {
            auto exc = FutureError{FutureErrorCode::broken_promise};
            shared_state->set_exception(std::make_exception_ptr(exc));
        }
    }
}

template <typename Type>
Future<Type> Promise<Type>::get_future() {
    return Future<Type>{this->shared_state};
}

template <typename Type>
void Promise<Type>::set_value(const Type& value) {
    this->check_shared_state();
    this->shared_state->set_value(value);
}

template <typename Type>
void Promise<Type>::set_value(Type&& value) {
    this->check_shared_state();
    this->shared_state->set_value(std::move(value));
}

template <typename Type>
template <typename... Args>
void Promise<Type>::set_value(sharp::emplace_construct::tag_t,
                              Args&&... args) {
    this->check_shared_state();
    this->shared_state->set_value(std::forward<Args>(args)...);
}

template <typename Type>
template <typename U, typename... Args>
void Promise<Type>::set_value(sharp::emplace_construct::tag_t,
                              std::initializer_list<U> il, Args&&... args) {
    this->check_shared_state();
    this->shared_state->set_value(il, std::forward<Args>(args)...);
}

template <typename Type>
void Promise<Type>::set_exception(std::exception_ptr ptr) {
    this->check_shared_state();
    this->shared_state->set_exception(ptr);
}

template <typename Type>
void Promise<Type>::check_shared_state() const {
    if (!this->shared_state) {
        throw FutureError{FutureErrorCode::no_state};
    }
}

} // namespace sharp
