/**
 * @file RecursiveMutex.hpp
 * @author Aaryaman Sagar (rmn100@gmail.com)
 *
 * Contains a simple implementation of a recursive mutex implemented around
 * the C++ standard library monitors
 */

#pragma once

#include <condition_variable>
#include <mutex>
#include <thread>

namespace sharp {

class RecursiveMutex {
public:
    /**
     * Acquire the lock held internally recursively, so if the same thread
     * that has the lock requests the lock, then this will not block
     */
    void lock();

    /**
     * release the lock
     */
    void unlock();

    /**
     * Non blocking try lock method that returns a true if we were able to
     * acquire the lock and false otherwise
     */
    bool try_lock();

private:
    /**
     * Private functions.  These are not gated with a mutex and should only be
     * called from within locked contexts
     */
    void acquire_lock();
    bool is_lock_acquirable() const;

    /**
     * private monitor
     */
    std::condition_variable cv;
    std::mutex mtx;

    /**
     * The thread id for the thread holding the lock currently
     */
    std::thread::id thread_holding_lock;

    /**
     * A counter to keep track of whether the lock has been released or not
     */
    int counter{0};
};

} // namespace sharp
