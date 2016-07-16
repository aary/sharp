/**
 * @file LockedData.hpp
 * @author Aaryaman Sagar (rmn100@gmail.com)
 *
 * This file contains a simple abstraction that halps in maintaining data that
 * is meant to be shared across different threads in a more elegant way than
 * maintaining an object along with its mutex
 */

#pragma once

#include <iostream>
#include <atomic>
#include <mutex>
#include <queue>

namespace sharp {

template <typename T>
class LockedData {
protected:
    /*
     * The data
     */
    T data;

    /*
     * the atomic variable used in locking
     */
    std::atomic<bool> guard;

public:

    /*
     * Execute the function passed atomically, using lambdas here makes using this
     * convenient, so the following can be used as so
     *
     * LockedData<queue<int>> tsq;
     * // Atomic operation
     * tsq.execute_atomic([] (std::queue<int>& queue_in) {
     *     queue_in.push(new ucontext_t);
     * });
     */
    template <typename Func>
    decltype(auto) execute_atomic(Func func) -> decltype(func(this->data));
    template <typename Func>
    decltype(auto) execute_unatomic(Func func) -> decltype(func(this->data));

    /*
     * RAII Style lock acquire and release mechanism, simply create this in front
     * of the code that needs locking
     */
    struct AcquireLock {
        AcquireLock() {
            while(this->guard.exchange(true));
        }

        ~AcquireLock() {
            this->guard.store(false);
        }
    };
};

template <typename T>
template <typename Func>
decltype(auto) LockedData<T>::execute_atomic(Func func)
    -> decltype(func(this->data)) {

        // RAII style lock acquire and release, this function cannot work without
        // this when decltype(func()) => void
        AcquireLock acquire_lock;

            return func(this->data);

        // Lock released here after the return statement, or after the critical
        // section of code in func() has been executed
}

template <typename T>
template <typename Func>
decltype(auto) LockedData<T>::execute_unatomic(Func func)
    -> decltype(func(this->data)) {
        return func(this->data);
}

} // namespace sharp
