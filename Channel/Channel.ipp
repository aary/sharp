#include <sharp/Channel/Channel.hpp>
#include <sharp/Defer/Defer.hpp>
#include <sharp/Functional/Functional.hpp>
#include <sharp/Traits/Traits.hpp>
#include <sharp/ForEach/ForEach.hpp>

#include <condition_variable>
#include <initializer_list>
#include <mutex>
#include <exception>
#include <vector>

namespace sharp {

template <typename Type, typename Mutex, typename Cv>
Channel<Type, Mutex, Cv>::Channel(int b) : buffer_length{b}, state{State{b}} {}

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
    auto state = this->state.lock();

    // increment the number of open slots before going to bed because there is
    // now a read which is possibly waiting for a write to go through on the
    // other end
    ++(state->open_slots);

    // sleep af if the elements queue is empty
    state.wait([](auto& state) {
        return !state.elements.empty();
    });

    // return the first element and pop af
    auto deferred = sharp::defer([&]() { state->elements.pop(); });
    return std::move(state->elements.front());
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
    return this->state.synchronized([](auto& state) -> sharp::Try<Type> {
        if (!state.elements.empty()) {
            // return the first element and pop af
            auto deferred = sharp::defer([&]() { state.elements.pop(); });
            return std::move(state.elements.front());
        } else {
            return nullptr;
        }
    });
}

template <typename Type, typename Mutex, typename Cv>
template <typename Func>
bool Channel<Type, Mutex, Cv>::try_send_impl(Func enqueue) {
    return this->state.synchronized([enqueue](auto& state) {
        if (state.open_slots) {
            // if there is space then enqueue the element and decrement the
            // number of open slots for sends
            enqueue(state.elements);
            --(state.open_slots);
            return true;
        }

        return false;
    });
}

template <typename Type, typename Mutex, typename Cv>
template <typename Func>
void Channel<Type, Mutex, Cv>::send_impl(Func enqueue) {
    auto state = this->state.lock();

    // wait for open slots to be non 0
    state.wait([](auto& state) {
        return state.open_slots != 0;
    });

    // then decrement the open slots and write to the queue af
    enqueue(state->elements);
    --(state->open_slots);
}

} // namespace sharp
