#include <sharp/Threads/RecursiveMutex.hpp>

#include <thread>
#include <mutex>
#include <condition_variable>
#include <stdexcept>

namespace sharp {

void RecursiveMutex::lock() {
    // acquire the mutex, increment the counter and then wait on the
    // condition_variable if this is not the thread that has the lock
    auto lck = std::unique_lock<std::mutex>{this->mtx};

    // dont block if this is the thread that has the lock, or if the value of
    // counter is 0 meaning that no thread is holding the lock
    if (this->is_lock_acquirable()) {
        this->acquire_lock();
        return;
    }

    // else block on the condition_variable
    while (counter) {
        this->cv.wait(lck);
    }

    this->acquire_lock();
}

void RecursiveMutex::unlock() {
    auto lck = std::unique_lock<std::mutex>{this->mtx};

    // check if the thread that releases the lock is the current thread, if it
    // is not then do not risk undefined POSIX behavior and throw an
    // exception, this is justified becasue the mutex already knows which
    // thread has the lock
    if (!counter) {
        throw std::runtime_error{"sharp::RecursiveMutex::unlock() called when "
            "the mutex is already unlocked"};
    }
    if (std::this_thread::get_id() != this->thread_holding_lock) {
        throw std::runtime_error{"sharp::RecursiveMutex::unlock() called from "
            "a thread that does not hold the lock"};
    }

    // decrement the counter and then notify on the condtion variable if the
    // counter has reached 0
    --counter;
    if (!counter) {
        this->cv.notify_one();
    }
}

bool RecursiveMutex::try_lock() {
    auto lck = std::unique_lock<std::mutex>{this->mtx};

    // either acquire the lock if the current thread is holding the lock
    // already or if the lock is free
    if (this->is_lock_acquirable()) {
        this->acquire_lock();
        return true;
    }

    return false;
}

bool RecursiveMutex::is_lock_acquirable() const {
    return this->thread_holding_lock == std::this_thread::get_id() || !counter;
}

void RecursiveMutex::acquire_lock() {
    ++counter;
    this->thread_holding_lock = std::this_thread::get_id();
}

} // namespace sharp
