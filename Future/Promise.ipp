#include <initializer_list>
#include <exception>
#include <memory>

#include <sharp/Future/Future.hpp>
#include <sharp/Future/Promise.hpp>

namespace sharp {

template <typename Type>
Promise<Type>::Promise() : shared_state{std::make_shared<FutureImpl>()} {}

template <typename Type>
Promise<Type>::~Promise() {
}

template <typename Type>
Future<Type> Promise<Type>::get_future() {
}

template <typename Type>
void Promise<Type>::set_value(const Type& value) {
    this->shared_state.set_value(value);
}

template <typename Type>
void Promise<Type>::set_value(Type&& value) {
    this->shared_state.set_value(std::move(value));
}

template <typename Type>
template <typename... Args>
void Promise<Type>::set_value(sharp::emplace_construct::tag_t,
                              Args&&... value) {
    this->shared_state.set_value(std::forward<Args>(args)...);
}

template <typename Type>
template <typename U, typename... Args>
void Promise<Type>::set_value(sharp::emplace_construct::tag_t,
                              std::initializer_list<U> il, Args&&... args) {
    this->shared_state.set_value(il, std::forward<Args>(args)...);
}

template <typename Type>
void Promise<Type>::set_exception(std::exception_ptr ptr) {
}

} // namespace sharp
