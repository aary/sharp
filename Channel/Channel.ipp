#include <condition_variable>
#include <mutex>

#include <sharp/Channel/Channel.hpp>

namespace sharp {

namespace detail {

    /**
     * Cast the union to the type passed in and return the reference with the
     * same reference qualifiers
     */
    template <typename Type, typename AlignedUnion>
    sharp::MatchReference_t<AlignedUnion&&, Type> union_cast(AlignedUnion&& u) {
        using TypeToCast = std::remove_reference_t<
            sharp::MatchReference_t<AlignedUnion&&, Type>>;
        auto* ptr = reinterpret_cast<TypeToCast*>(&u);
        return *ptr;
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
        : buffer_length{buffer_length_in} {}

template <typename Type>
template <typename TypeForward, typename Func>
void Channel<Type>::send_impl(Func&& enqueue_func) {

    assert(this->num_readers_waiting >= 0);
    assert(this->elements.size() <= this->buffer_length);
    auto lck = std::unique_lock<std::mutex>{this->mtx};

    // increment the number of waiting writes, at this stage we don't know if
    // the write will block, so assume that this write will wait
    ++this->waiting_writes;

    // wait while there is no space in the queue, or while there are no
    // readers waiting for the value to be sent across
    while (this->should_write_wait()) {
        this->write_cv.wait(lck);
    }

    // this write has gone through now
    --this->waiting_writes;

    // at this point there is a reader, or there is an empty spot in the
    // queue, so put the element into the queue and signal any readers
    // that there has been a read
    //
    // It doesn't matter if the sender was signalled to carry on because
    // there was space in the queue or if there was a reader to read the
    // value out of the channel, the enqueue_element simply puts the
    // element into the queue and then the reader will pop out of the
    // queue
    std::forward<Func>(enqueue_func)();

    // signal one reader that there is an element in the queue
    this->read_cv.notify_one();
}

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
void Channel<Type>::send_exception(std::exception_ptr p) {
    this->send_impl([this, &]() {
        this->queue.push_back(ValueOrException{});
        this->queue.back().exc_ptr = p;
        this->queue.back().has_exception = true;
    });
}

template <typename Type>
Type Channel<Type>::read() {

    assert(this->num_readers_waiting >= 0);

    // acquire the lock and wait until there is a value to read in the
    // queue
    auto lck = std::unique_lock<std::mutex>{this->mtx};
    while(this->queue.empty()) {
        this->read_cv.wait(lck);
    }

    // remove the front of the queue after the return value has been
    // constructed
    //
    // TODO replace this with a DEFER() after implementing it
    using QueueType = std::decay_t<decltype(this->queue)>;
    struct PopQueueOnDestruction {
        PopQueueOnDestruction(QueueType& queue_in) : q{queue_in} {}
        ~PopQueueOnDestruction() {
            // if the front of the queue does not have an exeption then
            // the value from the queue will have been returned by move()
            // in the last line of this function, so destroy whatever was
            // in a "valid but unspecified state"
            if (!q.front().has_exception) {
                (*reinterpret_cast<Type*>(q.front().storage))->~Type();
            }
            q.pop_front();
        }
        QueueType& q;
    };
    auto raii_pop = PopQueueOnDestruction{this->queue};

    // then read a value out of the channel, check to see if its an
    // exception, if it is then throw it
    if (this->queue.front().has_exception) {
        std::rethrow_exception(value.exc_ptr);
    }

    // return the value by moving it into the return value, which is then
    // a prvalue which will likely be elided (definitely elided in the
    // C++17 standard)
    //
    // If i had instead popped the value before, stored the previous
    // front() of the queue in a local variable and then returned the
    // value by reinterpret_cast-ing, the return value would not be a
    // prvalue and would be an xvalue which could not be elided into the
    // return value.  Therefore there would be two moves.  Not optimal!
    // Hence the raii pop does what we need here
    else return std::move(
            *reinterpret_cast<Type*>(this->queue.front().storage));
}

} // namespace sharp
