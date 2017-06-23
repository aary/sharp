#include <condition_variable>
#include <mutex>
#include <exception>

#include <sharp/Channel/Channel.hpp>
#include <sharp/Defer/Defer.hpp>

namespace sharp {

namespace detail {

    /**
     * Get the value in the node by reference or throw an exception
     */
    template <typename NodeType>
    typename std::decay_t<NodeType>::value_type& try_value(NodeType& node) {

        // if the node contains an exception throw it otherwise return the
        // value in the node by reference
        if (node.is_exception) {
            std::rethrow_exception(
                    *(reinterpret_cast<std::exception_ptr*>(&node.storage)));
        } else {
            using Type = typename std::decay_t<NodeType>::value_type;
            return *reinterpret_cast<Type*>(&node.storage);
        }
    }

} // namespace detail

/**
 * The constructor for the channel sets the inner buffer length data
 * member to be the one that was passed in, and then subsequently the
 * channel will use the data member and the size of the queue to determine
 * if a read or write should block
 */
template <typename Type>
Channel<Type>::Channel(int buffer_length_in = 0)
        : buffer_length{buffer_length_in},
          number_open_slots{buffer_length_in} {}

template <typename Type>
void Channel<Type>::send(const Type& value) {
    this->send_impl([this, &]() {
        this->enqueue_element(value);
    });
}

template <typename Type>
void Channel<Type>::send(Type&& value) {
    this->send_impl([this, &]() {
        this->enqueue_element(std::move(value));
    });
}

template <typename Type>
template <typename... Args>
void Channel<Type>::send(sharp::emplace_construct::tag_t, Args&&... args) {
    this->send_impl([this, &]() {
        this->enqueue_element(std::forward<Args>(args)...);
    });
}

template <typename Type>
template <typename U, typename... Args>
void Channel<Type>::send(sharp::emplace_construct::tag_t,
                         std::initializer_list<U> ilist,
                         Args&&... args) {
    this->send_impl([this, &]() {
        this->enqueue_element(ilist, std::forward<Args>(args)...);
    });
}

template <typename Type>
void Channel<Type>::send_exception(std::exception_ptr ptr) {
    this->send_impl([this, ptr]() {
        this->queue.emplace_back();
        new (&this->queue.back().storage) std::exception_ptr{ptr};
        this->queue.back().is_exception = true;
    });
}

template <typename Type>
Type Channel<Type>::read() {

    assert(this->num_readers_waiting >= 0);

    // acquire the lock and wait until there is a value to read in the
    // queue
    auto lck = std::unique_lock<std::mutex>{this->mtx};

    [this]() {
        // if there is a new reader then there can be another write that can
        // possibly go through
        ++this->number_open_slots;
        auto deferred = sharp::defer([this]() {
            --this->number_open_slots;
        });

        // wait if the read cannot proceed
        while (!this->can_read_proceed()) {
            this->write_cv.notify_one();
            this->read_cv.wait(lck);
        }
    }();

    // then read a value out of the channel, check to see if its an
    // exception, if it is then throw it and then pop the front of the queue
    auto& front = this->queue.front();
    auto deferred = sharp::defer([this]() {
        this->queue.pop_front();
    });
    return std::move(try_value(front));
}

template <typename Type>
template <typename TypeForward, typename Func>
void Channel<Type>::send_impl(Func&& enqueue_func) {

    assert(this->num_readers_waiting >= 0);
    assert(this->elements.size() <= this->buffer_length);
    auto lck = std::unique_lock<std::mutex>{this->mtx};

    // wait while there can is a non-zero number of open slots to write to
    while (!this->can_write_proceed()) {
        this->write_cv.wait(lck);
    }

    // at this point the write can go through so write and then signal one
    // reader
    std::forward<Func>(enqueue_func)();
    --this->number_open_slots;
    this->read_cv.notify_one();
}

template <typename... Args>
void Channel<Type>::enqueue_element(Args&&... args) {
    // add an element to the back of the queue first and then construct it in
    // place
    this->elements.emplace_back();
    new (&this->elements.back()) Type{std::forward<Args>(args)...};
}

template <typename U, typename... Args>
void Channel<Type>::enqueue_element(std::initializer_list<U> ilist,
                                    Args&&... args) {
    // add an element to the back of the queue first
    this->elements.emplace_back();
    new (&this->elements.back()) Type{ilist, std::forward<Args>(args)...};
}

template <typename Type>
bool Channel<Type>::can_read_proceed() const {
    // reads should wait only if there are no elements in the internal queue,
    // i.e. if a write has not already gone through
    return !this->elements.empty();
}

template <typename Type>
bool Channel<Type>::can_write_proceed() const {
    // a write can only proceed if there is a slot left in the buffer or if
    // there is a waiting reader, this corresponds 1-1 with the number of open
    // slots, so just compare that to 0
    return this->number_open_slots != 0;
}

} // namespace sharp
