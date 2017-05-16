#pragma once

#include <exception>
#include <condition_variable>
#include <mutex>
#include <initializer_list>
#include <utility>
#include <cassert>

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
        auto lck = std::unique_lock<std::mutex>{this->mtx};

        // construct the object into place and change the value of the state
        // variable
        this->check_set_value();
        new (&this->get_value()) Type{std::forward<Args>(args)...};
        this->after_set_value();

        // execute the callback if there is one set
        this->execute_callback(lck);
    }

    template <typename Type>
    template <typename U, typename... Args>
    void FutureImpl<Type>::set_value(std::initializer_list<U> il,
                                     Args&&... args) {

        // acquire the lock and then construct the data item in the storage
        auto lck = std::unique_lock<std::mutex>{this->mtx};

        // construct the object into place and change the value of the state
        // variable
        this->check_set_value();
        new (&this->get_value()) Type{il, std::forward<Args>(args)...};
        this->after_set_value();

        // execute the callback if there is one set
        this->execute_callback(lck);
    }

    template <typename Type>
    void FutureImpl<Type>::set_exception(std::exception_ptr ptr) {

        // acquire the lock and then set the exception
        auto lck = std::unique_lock<std::mutex>{this->mtx};

        // construct the exception into place and change the value of the
        // booleans
        this->check_set_value();
        new (&this->get_exception_ptr()) std::exception_ptr(ptr);
        this->after_set_exception();

        // execute the callback if there is one set
        this->execute_callback(lck);
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
        return std::move(this->get_value());
    }

    template <typename Type>
    void FutureImpl<Type>::test_and_set_retrieved_flag() {
        if (this->retrieved.test_and_set()) {
            throw FutureError{FutureErrorCode::future_already_retrieved};
        }
    }

    template <typename Type>
    template <typename Func>
    void FutureImpl<Type>::add_callback(Func&& func) {

        auto lck = std::unique_lock<std::mutex>{this->mtx};

        // this should not be called twice, and will only be called internally
        // so assert
        assert(!this->callback);

        // if the value or exception has already been set then call the
        // functor now
        if (this->state.load() != FutureState::NotFulfilled) {
            lck.unlock();
            std::forward<Func>(func)(*this);
        } else {
            // otherwise pack it up into a callback and store the callback
            this->callback = std::forward<Func>(func);
        }
    }

    template <typename Type>
    void FutureImpl<Type>::check_get() const {
        if (this->state.load() == FutureState::ContainsException) {
            std::rethrow_exception(this->get_exception_ptr());
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
    void FutureImpl<Type>::after_set_value() {
        this->state.store(FutureState::ContainsValue);
        this->cv.notify_one();
    }

    template <typename Type>
    void FutureImpl<Type>::after_set_exception() {
        this->state.store(FutureState::ContainsException);
        this->cv.notify_one();
    }

    template <typename Type>
    void FutureImpl<Type>::execute_callback(std::unique_lock<std::mutex>& lck) {
        assert(lck.owns_lock());
        if (!this->callback) {
            return;
        }
        lck.unlock();
        this->callback(*this);
    }

    template <typename Type>
    bool FutureImpl<Type>::is_ready() const noexcept {
        return (this->state.load() != FutureState::NotFulfilled);
    }

    template <typename Type>
    bool FutureImpl<Type>::contains_exception() const {
        std::lock_guard<std::mutex> lck{this->mtx};
        return (this->state.load() == FutureState::ContainsException);
    }

    template <typename Type>
    std::exception_ptr& FutureImpl<Type>::get_exception_ptr() {
        auto* exception_ptr
            = reinterpret_cast<std::exception_ptr*>(&this->storage);
        return *exception_ptr;
    }

    template <typename Type>
    const std::exception_ptr& FutureImpl<Type>::get_exception_ptr() const {
        auto* exception_ptr
            = reinterpret_cast<const std::exception_ptr*>(&this->storage);
        return *exception_ptr;
    }

    template <typename Type>
    Type& FutureImpl<Type>::get_value() {
        auto* object_ptr = reinterpret_cast<Type*>(&this->storage);
        return *object_ptr;
    }

}

} // namespace sharp
