#pragma once

#include <exception>
#include <condition_variable>
#include <mutex>
#include <initializer_list>
#include <utility>

#include <sharp/Future/FutureError.hpp>
#include <sharp/Future/detail/FutureImpl.ipp>

namespace sharp {

namespace detail {

    template <typename Type>
    void FutureImpl<Type>::wait() const {

        // double checked locking, if the value has been set then just return
        // because the value has been set; if not then wait on a mutex the
        // regular way until the value has been set
        if (this->state.load() != FutureState::NotFulfilled) {
            return;
        }

        // if the check above failed then the value has not been set, so sleep
        // until the value is set
        auto lck = std::unique_lock<std::mutex>{this->mtx};
        while (this->state.load() == FutureState::NotFulfilled) {
            this->cv.wait(lck);
        }
    }

    template <typename Type>
    template <typename... Args>
    void FutureImpl<Type>::set_value(Args&&... args) {

        // acquire the lock and then construct the data item in the storage
        std::lock_guard<std::mutex> lck{this->mtx};

        // construct the object into place and change the value of the state
        // variable
        this->check_set_value();
        new (this->get_object_storage()) Type{std::forward<Args>(args)...};
        this->after_set_value();
    }

    template <typename Type>
    template <typename U, typename... Args>
    void FutureImpl<Type>::set_value(std::initializer_list<U> il,
                                     Args&&... args) {

        // acquire the lock and then construct the data item in the storage
        std::lock_guard<std::mutex> lck{this->mtx};

        // construct the object into place and change the value of the state
        // variable
        this->check_set_value();
        new (this->get_object_storage()) Type{il, std::forward<Args>(args)...};
        this->after_set_value();
    }

    template <typename Type>
    void FutureImpl<Type>::set_exception(std::exception_ptr ptr) {

        // acquire the lock and then set the exception
        std::lock_guard<std::mutex> lck{this->mtx};

        // construct the exception into place and change the value of the
        // booleans
        this->check_set_value();
        new (this->get_exception_storage()) std::exception_ptr(ptr);
        this->after_set_exception();
    }

    template <typename Type>
    Type FutureImpl<Type>::get() {

        // first wait for the result to be ready, and then cast and return the
        // value
        this->wait();

        // grab a lock and then do stuff
        std::lock_guard<std::mutex> lck(this->mtx);

        // check and throw an exception if the future has already been
        // fulfilled, and then if not store state and return the moved value
        this->check_get();
        return std::move(*this->get_object_storage());
    }

    template <typename Type>
    void FutureImpl<Type>::test_and_set_retrieved_flag() {
        if (this->retrieved.test_and_set()) {
            throw FutureError{FutureErrorCode::future_already_retrieved};
        }
    }

    template <typename Type>
    void FutureImpl<Type>::check_get() const {

        auto state = this->state.load();
        if (state == FutureState::ContainsException) {
            std::rethrow_exception(*this->get_exception_storage());
        }
    }

    template <typename Type>
    void FutureImpl<Type>::check_set_value() const {

        auto state = this->state.load();
        if (state == FutureState::ContainsException
                || state == FutureState::ContainsValue) {
            throw FutureError{FutureErrorCode::promise_already_satisfied};
        }
    }

    template <typename Type>
    void FutureImpl<Type>::after_set_value() const {
        this->state.store(FutureState::ContainsValue);
        this->cv.notify_one();
    }

    template <typename Type>
    void FutureImpl<Type>::after_set_exception() const {
        this->state.store(FutureState::ContainsException);
        this->cv.notify_one();
    }

    template <typename Type>
    Type* FutureImpl<Type>::get_object_storage() {
        return reinterpret_cast<Type*>(&this->storage);
    }

    template <typename Type>
    std::exception_ptr* FutureImpl<Type>::get_exception_storage() {
        return reinterpret_cast<std::exception_ptr*>(&this->storage);
    }

    template <typename Type>
    const std::exception_ptr* FutureImpl<Type>::get_exception_storage() const {
        return reinterpret_cast<const std::exception_ptr*>(&this->storage);
    }

    template <typename Type>
    bool FutureImpl<Type>::is_ready() const noexcept {
        auto state = this->state.load();
        return (state == FutureState::ContainsValue)
            || (state == FutureState::ContainsException);
    }
}

} // namespace sharp
