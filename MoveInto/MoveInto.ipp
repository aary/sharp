#pragma once

#include <sharp/MoveInto/MoveInto.hpp>

#include <memory>

namespace sharp {

template <typename T>
MoveInto<T>::MoveInto(T&& in) : instance{std::move(in)} {}

template <typename T>
T& MoveInto<T>::operator*() & {
    return this->instance;
}

template <typename T>
const T& MoveInto<T>::operator*() const & {
    return this->instance;
}

template <typename T>
T&& MoveInto<T>::operator*() && {
    return std::move(this->instance);
}

template <typename T>
const T&& MoveInto<T>::operator*() const && {
    return std::move(this->instance);
}

template <typename T>
T* MoveInto<T>::operator->() {
    return std::addressof(this->instance);
}

template <typename T>
const T* MoveInto<T>::operator->() const {
    return std::addressof(this->instance);
}

} // namespace sharp
