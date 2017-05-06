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

        // double checked locking, if the value has been set then just return,
        // because the value has been set, if not then wait
        if (this->is_set.load()) {
            return;
        }

        // if the check above failed then the value has not been set, so sleep
        // until the value is set
        auto lck = std::unique_lock<std::mutex>{this->mtx};
        while (!this->is_set) {
            this->cv.wait(lck);
        }
    }

    template <typename Type>
    template <typename... Args>
    void FutureImpl<Type>::set_value_impl(Args&&... args) {

        // acquire the lock and then construct the data item in the storage
        auto lck = std::unique_lock<std::mutex>{this->mtx};
        auto* ptr_type = reinterpret_cast<Type*>(this->storage);
        new (ptr_type) Type{std::forward<Args>(args)...};

        // atomically change the value of the is_set variable.  This is an
        // atomic variable because it is accessed outside the protection of
        // the mutex, see wait() for more
        is_set.store(true);
    }

    template <typename Type>
    template <typename... Args>
    void FutureImpl<Type>::set_value_impl(std::initializer_list<U> il,
                                          Args&&... args) {

        // acquire the lock and then construct the data item in the storage
        auto lck = std::unique_lock<std::mutex>{this->mtx};
        auto* ptr_type = reinterpret_cast<Type*>(this->storage);
        new (ptr_type) Type{il, std::forward<Args>(args)...};

        // atomically change the value of the is_set variable.  This is an
        // atomic variable because it is accessed outside the protection of
        // the mutex, see wait() for more
        is_set.store(true);
    }

    template <typename Type>
    void FutureImpl<Type>::set_value(const Type& value) {
        this->set_value_impl(value);
    }

    template <typename Type>
    void FutureImpl<Type>::set_value(Type&& value) {
        this->set_value_impl(std::move(value));
    }

    template <typename Type>
    template <typename... Args>
    void FutureImpl<type>::set_value(sharp::emplace_construct::tag_t,
                                     Args&&... args) {
        this->set_value_impl(std::forward<Args>(args)...);
    }

    template <typename Type>
    template <typename U, typename... Args>
    void FutureImpl<type>::set_value(sharp::emplace_construct::tag_t,
                                     std::initializer_list<U> il,
                                     Args&&... args) {
        this->set_value_impl(il, std::forward<Args>(args)...);
    }
}

} // namespace sharp
