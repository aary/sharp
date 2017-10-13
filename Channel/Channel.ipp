#include <sharp/Channel/Channel.hpp>
#include <sharp/Defer/Defer.hpp>
#include <sharp/Functional/Functional.hpp>
#include <sharp/Traits/Traits.hpp>
#include <sharp/ForEach/ForEach.hpp>

#include <condition_variable>
#include <intiializer_list>
#include <mutex>
#include <exception>
#include <vector>

namespace sharp {

namespace detail {

} // namespace detail

template <typename Type, typename Mutex, typename Cv>
Channel<Type, Mutex, Cv>::Channel(int b) : buffer_length{b} {}

template <typename Type, typename Mutex, typename Cv>
void Channel<Type, Mutex, Cv>::send(const Type& value) {
    this->send_impl([&](auto& elements) { elements.emplace(value); });
}

template <typename Type, typename Mutex, typename Cv>
void Channel<Type, Mutex, Cv>::send(Type&& v) {
    this->send_impl([&](auto& elements) { elements.emplace(std::move(v)); });
}

template <typename Type, typename Mutex, typename Cv>
template <typename... Args>
void Channel<Type, Mutex, Cv>::send(std::in_place_t, Args&&... args) {
    this->send_impl([&](auto& elements) {
        elements.emplace(std::in_place, std::forward<Args>(args)...);
    });
}

template <typename Type, typename Mutex, typename Cv>
template <typename U, typename... Args>
void Channel<Type, Mutex, Cv>::send(std::in_place_t,
                                    std::initializer_list<U> il,
                                    Args&&... args) {
    this->send_impl([&](auto& elements) {
        elements.emplace(std::in_place, il, std::forward<Args>(args)...);
    });
}

template <typename Type, typename Mutex, typename Cv>
bool Channel<Type, Mutex, Cv>::try_send(const Type& v) {
    return this->try_send_impl([&](auto& elements) { elements.emplace(v); });
}

template <typename Type, typename Mutex, typename Cv>
bool Channel<Type, Mutex, Cv>::try_send(Type&& v) {
    return this->try_send_impl([&](auto& elements) {
        elements.emplace(std::move(v));
    });
}

template <typename Type, typename Mutex, typename Cv>
Type Channel<Type, Mutex, Cv>::read() {
    return this->read_try().get();
}

template <typename Type, typename Mutex, typename Cv>
sharp::Try<Type> Channel<Type, Mutex, Cv>::read_try() {
    // wait for the elements to have an element and then read it
    auto elements = this->elements.lock();

    // increment the number of open slots before going to bed because there is
    // now a read
    ++(*this->open_slots().lock());

    // sleep af if the elements queue is empty
    elements.wait([](auto& elements) {
        return !elements.empty();
    });

    // return the first element and pop af
    auto deferred = sharp::defer([&]() { elements->pop(); });
    return std::move(elements->front());
}

template <typename Type, typename Mutex, typename Cv>
std::optional<Type> Channel<Type, Mutex, Cv>::try_read() {
    auto t = this->try_read_try();
    if (t.valid()) {
        return std::move(t).value();
    } else {
        return std::nullopt;
    }
}

template <typename Type, typename Mutex, typename Cv>
sharp::Try<Type> Channel<Type, Mutex, Cv>::try_read_try() {
    return this->elements.synchronized([](auto& elements) {
        if (!elements.empty()) {
            // return the first element and pop af
            auto deferred = sharp::defer([&]() { lock->pop(); });
            return std::move(lock->front());
        } else {
            return nullptr;
        }
    });
}

template <typename Type, typename Mutex, typename Cv>
template <typename Func>
bool try_send_impl(Func enqueue) {
    return this->open_slots.synchronized([](auto& open_slots) {
        if (open_slots) {
            enqueue(*this->elements.lock());
            --open_slots;
            return true;
        }
        return false;
    });
}

template <typename Type, typename Mutex, typename Cv>
template <typename Func>
void send_impl(Func enqueue) {
    auto open_slots = this->open_slots.lock();

    // wait for open slots to be non 0
    open_slots.wait([](auto& open_slots) {
        return open_slots != 0;
    });

    // then decrement the open slots and write to the queue af
    --(*open_slots);
    this->elements.synchronized([](auto& elements) {
        enqueue(elements);
    });
}

} // namespace sharp
