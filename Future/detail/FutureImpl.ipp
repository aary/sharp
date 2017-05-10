#pragma once

#include <exception>
#include <condition_variable>
#include <mutex>
#include <initializer_list>
#include <utility>

#include <sharp/Future/detail/FutureImpl.ipp>

namespace sharp {

namespace detail {

    template <typename Type>
    void FutureImpl<Type>::wait() const {

        // double checked locking, if the value has been set then just return
        // because the value has been set; if not then wait on a mutex the
        // regular way until the value has been set
        if (this->is_set.load()) {
            return;
        }

        // if the check above failed then the value has not been set, so sleep
        // until the value is set
        auto lck = std::unique_lock<std::mutex>{this->mtx};
        while (!this->is_set.load()) {
            this->cv.wait(lck);
        }
    }

    template <typename Type>
    template <typename... Args>
    void FutureImpl<Type>::set_value(Args&&... args) {

        // acquire the lock and then construct the data item in the storage
        auto lck = std::unique_lock<std::mutex>{this->mtx};
        auto* ptr_type = reinterpret_cast<Type*>(&this->storage);
        new (ptr_type) Type{std::forward<Args>(args)...};

        // atomically change the value of the is_set variable.  This is an
        // atomic variable because it is accessed outside the protection of
        // the mutex, see wait() for more
        is_set.store(true);
    }

    template <typename Type>
    template <typename... Args>
    void FutureImpl<Type>::set_value(std::initializer_list<U> il,
                                     Args&&... args) {

        // acquire the lock and then construct the data item in the storage
        auto lck = std::unique_lock<std::mutex>{this->mtx};
        auto* ptr_type = reinterpret_cast<Type*>(&this->storage);
        new (ptr_type) Type{il, std::forward<Args>(args)...};

        // atomically change the value of the is_set variable.  This is an
        // atomic variable because it is accessed outside the protection of
        // the mutex, see wait() for more
        is_set.store(true);
    }

    template <typename Type>
    void FutureImpl<Type>::set_exception(std::exception_ptr ptr) {

        // acquire the lock and then set the exception
        auto lck = std::unique_lock<std::mutex>{this->mtx};
        auto* ptr_exception = reinterpret_cast<std::exception_ptr*>(
                &this->storage);
        *ptr_exception = ptr;
        this->contains_exception = true;
    }

    template <typename Type>
    Type FutureImpl<Type>::get() const {

        // first wait for the result to be ready, and then cast and return the
        // value
        this->wait();
        auto* ptr_type = reinterpret_cast<Type*>(this->storage);
        return std::move(*ptr_type);
    }
}

} // namespace sharp
