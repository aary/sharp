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

namespace channel_detail {

    /**
     * Concepts(ish)
     */
    /**
     * Enable if the function is a read function, i.e. it will execute a read
     * on the underlying channel
     */
    template <typename Channel, typename Func>
    using EnableIfRead = sharp::void_t<decltype(
        std::declval<Func>()(std::declval<typename Channel::value_type>()))>;
    template <typename Channel, typename Func>
    using EnableIfWrite = sharp::void_t<decltype(std::declval<Func>())>;

    /**
     * Specializations for the make_case function that wrap the callable
     * around a speculative reader/writer that 1 operates under the assumption
     * that the channel in question is already locked and 2 succeeds only when
     * either the read or write could go through
     */
    template <typename Channel, typename Func,
              EnableIfRead<Channel, Func>* = nullptr>
    auto make_case_impl(Channel& channel, Func&& func) {
        // returns a pair of the channel itself and a callable that accepts
        // the state of the concurrent object as a parameter and if a read can
        // succeed, it reads from the channel and invokes the fucntion passed
        // with the read value as a parameter
        return std::make_pair(std::ref(channel), [&](auto& state) {
            if (state.can_read_succeed()) {
                std::forward<Func>(func)(state.read());
                return true;
            }
            return false;
        });
    }
    template <typename Channel, typename Func,
              EnableIfWrite<Channel, Func>* = nullptr>
    auto make_case_impl(Channel& channel, Func&& func) {
        // return a pair of the channel itself and a callable that writes the
        // value into the channel if the channel is ready
        return std::make_pair(std::ref(channel), [&](auto& state) {
            if (state.can_write_succeed()) {
                state.elements.emplace(std::forward<Func>(func)());
                return true;
            }
            return false;
        });
    }
} // namespace channel_detail

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
        return state.can_read_succeed();
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
            return state.read();
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
        return state.can_write_succeed();
    });

    // then decrement the open slots and write to the queue af
    enqueue(state->elements);
    --(state->open_slots);
}

template <typename ChannelType, typename Func>
auto make_case(ChannelType& channel, Func&& func) {
    return channel_detail::make_case_impl(channel, std::forward<Func>(func));
}

template <typename Type, typename Mutex, typename Cv>
bool Channel<Type, Mutex, Cv>::State::can_read_succeed() const noexcept {
    return !this->elements.empty();
}

template <typename Type, typename Mutex, typename Cv>
bool Channel<Type, Mutex, Cv>::State::can_write_succeed() const noexcept {
    return this->open_slots != 0;
}

template <typename Type, typename Mutex, typename Cv>
auto Channel<Type, Mutex, Cv>::State::read() {
    // return the first element and pop af
    auto deferred = sharp::defer([&]() { this->elements.pop(); });
    return std::move(this->elements.front());
}

} // namespace sharp
