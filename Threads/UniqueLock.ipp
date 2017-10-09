#pragma once

#include <sharp/Threads/UniqueLock.hpp>

#include <mutex>
#include <stdexcept>
#include <system_error>
#include <type_traits>

namespace sharp {
namespace threads_detail {

    /**
     * Concepts(ish)
     */
    /**
     * Enable if the mutex is std::mutex
     */
    template <typename Mutex>
    using EnableIfStdMutex = std::enable_if_t<
        std::is_same<std::decay_t<Mutex>, std::mutex>::value>;
    /**
     * Enable if the lock function returns a boolean
     */
    template <typename Mutex, typename Locker>
    using EnableIfReturnsBool = std::enable_if_t<
        std::is_same<decltype(std::declval<Locker>()(std::declval<Mutex>())),
        bool>::value>;

    /**
     * Wrap the locking function to always return a boolean indicating whether
     * or not the mutex has been acquired
     */
    template <typename Mutex, typename Locker,
              typename = std::enable_if_t<true>>
    class WrapToReturnBool {
    public:
        WrapToReturnBool() = default;
        explicit WrapToReturnBool(Locker locker_in) : locker{locker_in} {}
        bool operator()(Mutex& mtx) {
            locker(mtx);
            return true;
        }
        Locker locker;
    };
    template <typename Mutex, typename Locker>
    class WrapToReturnBool<Mutex, Locker, EnableIfReturnsBool<Mutex, Locker>> {
    public:
        WrapToReturnBool() = default;
        explicit WrapToReturnBool(Locker locker_in) : locker{locker_in} {}
        bool operator()(Mutex& mtx) {
            return locker(mtx);
        }
        Locker locker;
    };

} // namespace threads_detail

struct DefaultLock {
    template <typename Mutex>
    void operator()(Mutex& mtx) {
        mtx.lock();
    }
};
struct DefaultUnlock {
    template <typename Mutex>
    void operator()(Mutex& mtx) {
        mtx.unlock();
    }
};
struct SharedLock {
    template <typename Mutex>
    void operator()(Mutex& mtx) {
        mtx.lock_shared();
    }
};
struct SharedUnlock {
    template <typename Mutex>
    void operator()(Mutex& mtx) {
        mtx.unlock_shared();
    }
};

template <typename Mutex, typename Lock, typename Unlock>
UniqueLock<Mutex, Lock, Unlock>::UniqueLock(UniqueLock&& other) noexcept {
    // GIVE THEM NOTHING, TAKE FROM THEM EVERYTHING!!
    this->swap(other);
}

template <typename Mutex, typename Lock, typename Unlock>
UniqueLock<Mutex, Lock, Unlock>::UniqueLock(Mutex& m) : mtx{&m} {
    this->owns_mutex = threads_detail::WrapToReturnBool<Mutex, Lock>{}(
            *this->mtx);
}

template <typename Mutex, typename Lock, typename Unlock>
UniqueLock<Mutex, Lock, Unlock>::UniqueLock(Mutex& m, std::defer_lock_t)
        : mtx{&m} {}

template <typename Mutex, typename Lock, typename Unlock>
UniqueLock<Mutex, Lock, Unlock>::UniqueLock(Mutex& m, std::try_to_lock_t)
        : mtx{&m} {
    this->owns_mutex = this->mtx->try_lock();
}

template <typename Mutex, typename Lock, typename Unlock>
UniqueLock<Mutex, Lock, Unlock>::UniqueLock(Mutex& m, std::adopt_lock_t)
        : mtx{&m}, owns_mutex{true} {}

template <typename Mutex, typename Lock, typename Unlock>
template <typename Rep, typename Period>
UniqueLock<Mutex, Lock, Unlock>::UniqueLock(
        Mutex& m,  const std::chrono::duration<Rep, Period>& duration)
        : mtx{&m} {
    this->owns_mutex = this->mtx->try_lock_for(duration);
}

template <typename Mutex, typename Lock, typename Unlock>
template <typename Clock, typename Duration>
UniqueLock<Mutex, Lock, Unlock>::UniqueLock(
        Mutex& m,
        const std::chrono::time_point<Clock, Duration>& time) : mtx{&m} {
    this->owns_mutex = this->mtx->try_lock_until(time);
}

template <typename Mutex, typename Lock, typename Unlock>
template <typename AcquireLock>
UniqueLock<Mutex, Lock, Unlock>::UniqueLock(Mutex& m, AcquireLock locker)
        : mtx{&m} {
    this->owns_mutex =
        threads_detail::WrapToReturnBool<Mutex, AcquireLock>{locker}(
                *this->mtx);
}

template <typename Mutex, typename Lock, typename Unlock>
UniqueLock<Mutex, Lock, Unlock>::~UniqueLock() {
    if (this->owns_mutex) {
        Unlock{}(*this->mtx);
    }
}

template <typename Mutex, typename Lock, typename Unlock>
UniqueLock<Mutex, Lock, Unlock>&
UniqueLock<Mutex, Lock, Unlock>::operator=(UniqueLock&& other) {
    // call the destructor for *this, unlocking the mtx if one was owned and
    // then take ownership of other
    this->~UniqueLock();
    this->swap(other);
}

template <typename Mutex, typename Lock, typename Unlock>
template <typename L>
void UniqueLock<Mutex, Lock, Unlock>::lock(L locker) {
    this->throw_if_no_mutex();
    this->throw_if_already_owned();
    this->owns_mutex = threads_detail::WrapToReturnBool<Mutex, L>{locker}(
            *this->mtx);
}

template <typename Mutex, typename Lock, typename Unlock>
template <typename U>
void UniqueLock<Mutex, Lock, Unlock>::unlock(U unlocker) {
    this->throw_if_no_mutex();
    unlocker(*this->mtx);
    this->owns_mutex = false;
}

template <typename Mutex, typename Lock, typename Unlock>
bool UniqueLock<Mutex, Lock, Unlock>::try_lock() {
    this->throw_if_no_mutex();
    this->throw_if_already_owned();
    this->owns_mutex = this->mtx->try_lock();
}

template <typename Mutex, typename Lock, typename Unlock>
template <typename Rep, typename Period>
bool UniqueLock<Mutex, Lock, Unlock>::try_lock_for(
        const std::chrono::duration<Rep, Period>& duration) {
    this->throw_if_no_mutex();
    this->throw_if_already_owned();
    this->owns_mutex = this->mtx->try_lock_for(duration);
}

template <typename Mutex, typename Lock, typename Unlock>
template <typename Clock, typename Duration>
bool UniqueLock<Mutex, Lock, Unlock>::try_lock_until(
        const std::chrono::time_point<Clock, Duration>& time) {
    this->throw_if_no_mutex();
    this->throw_if_already_owned();
    this->owns_mutex = this->mtx->try_lock_until(time);
}

template <typename Mutex, typename Lock, typename Unlock>
void UniqueLock<Mutex, Lock, Unlock>::swap(UniqueLock& other) noexcept {
    std::swap(this->mtx, other.mtx);
    std::swap(this->owns_mutex, other.owns_mutex);
}

template <typename Mutex, typename Lock, typename Unlock>
Mutex* UniqueLock<Mutex, Lock, Unlock>::release() noexcept {
    this->owns_mutex = false;
    auto to_return = this->mtx;
    this->mtx = nullptr;
    return to_return;
}

template <typename Mutex, typename Lock, typename Unlock>
Mutex* UniqueLock<Mutex, Lock, Unlock>::mutex() const noexcept {
    return this->mtx;
}

template <typename Mutex, typename Lock, typename Unlock>
bool UniqueLock<Mutex, Lock, Unlock>::owns_lock() const noexcept {
    return this->owns_mutex;
}

template <typename Mutex, typename Lock, typename Unlock>
UniqueLock<Mutex, Lock, Unlock>::operator bool() const noexcept {
    return this->owns_lock();
}

template <typename Mutex, typename Lock, typename Unlock>
void UniqueLock<Mutex, Lock, Unlock>::throw_if_no_mutex() const {
    if (!this->mtx) {
        throw std::system_error{
            std::make_error_code(std::errc::operation_not_permitted)};
    }
}

template <typename Mutex, typename Lock, typename Unlock>
void UniqueLock<Mutex, Lock, Unlock>::throw_if_already_owned() const {
    if (this->owns_mutex) {
        throw std::system_error{
            std::make_error_code(std::errc::resource_deadlock_would_occur)};
    }
}

} // namespace sharp
