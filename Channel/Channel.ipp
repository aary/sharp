#include <condition_variable>
#include <mutex>
#include <exception>
#include <vector>

#include <sharp/Channel/Channel.hpp>
#include <sharp/Defer/Defer.hpp>
#include <sharp/Functional/Functional.hpp>
#include <sharp/Traits/Traits.hpp>

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

    /**
     * Compile time wrapper to generate std::function objects that
     * speculatively try and read from the channel passed in
     */
    template <typename... SelectContexts>
    std::vector<sharp::Function<bool()>> make_try_funcs(SelectContexts&&...);

} // namespace detail

/**
 * The constructor for the channel sets the inner buffer length data
 * member to be the one that was passed in, and then subsequently the
 * channel will use the data member and the size of the queue to determine
 * if a read or write should block
 */
template <typename Type>
Channel<Type>::Channel(int buffer_length_in)
        : buffer_length{buffer_length_in},
          open_slots{buffer_length_in} {}

template <typename Type>
void Channel<Type>::send(const Type& value) {
    this->send_impl([&value, this]() {
        this->enqueue_element(value);
    });
}

template <typename Type>
void Channel<Type>::send(Type&& value) {
    this->send_impl([&value, this]() {
        this->enqueue_element(std::move(value));
    });
}

template <typename Type>
template <typename... Args>
void Channel<Type>::send(sharp::emplace_construct::tag_t, Args&&... args) {
    this->send_impl([&, this]() {
        this->enqueue_element(std::forward<Args>(args)...);
    });
}

template <typename Type>
template <typename U, typename... Args>
void Channel<Type>::send(sharp::emplace_construct::tag_t,
                         std::initializer_list<U> ilist,
                         Args&&... args) {
    this->send_impl([&, this]() {
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
std::optional<Type> Channel<Type>::try_read() {

    // acquire the lock and see if this read can proceed, if yes then pop from
    // the queue and return the value in an optional
    auto lck = std::unique_lock<std::mutex>{this->mtx};
    if (this->can_read_proceed()) {
        return this->do_read_no_block();
    } else {
        return std::nullopt;
    }
}

template <typename Type>
Type Channel<Type>::read() {

    // acquire the lock and wait until there is a value to read in the
    // queue
    auto lck = std::unique_lock<std::mutex>{this->mtx};

    // a reader is waiting, this means that there is another open slot in
    // the channel for a read to go through, when a write goes through it will
    // decrement the number of open slots again
    ++this->open_slots;

    // wait if the read cannot proceed
    while (!this->can_read_proceed()) {
        this->notify_waiting_writers();
        this->read_cv.wait(lck);
    }

    return this->do_read_no_block();
}

template <typename Type>
template <typename Func>
void Channel<Type>::send_impl(Func&& enqueue_func) {

    auto lck = std::unique_lock<std::mutex>{this->mtx};

    // wait while there can is a non-zero number of open slots to write to
    while (!this->can_write_proceed()) {
        this->write_cv.wait(lck);
    }

    // do the writing to the channel
    this->do_write_no_block(std::forward<Func>(enqueue_func));
}

template <typename Type>
Type Channel<Type>::do_read_no_block() {

    assert(this->can_read_proceed());

    // then read a value out of the channel, check to see if its an
    // exception, if it is then throw it and then pop the front of the queue
    auto deferred = sharp::defer([this]() {
        this->elements.pop_front();
        this->notify_waiting_readers();
    });
    return std::move(detail::try_value(this->elements.front()));
}

template <typename Type>
template <typename Func>
void Channel<Type>::do_write_no_block(Func&& enqueue_func) {

    assert(this->can_write_proceed());

    // at this point the write can go through so write and then signal one
    // reader
    std::forward<Func>(enqueue_func)();
    --this->open_slots;
    this->notify_waiting_readers();
}

template <typename Type>
template <typename... Args>
void Channel<Type>::enqueue_element(Args&&... args) {
    // add an element to the back of the queue first and then construct it in
    // place
    this->elements.emplace_back();
    new (&this->elements.back().storage) Type{std::forward<Args>(args)...};
}

template <typename Type>
template <typename U, typename... Args>
void Channel<Type>::enqueue_element(std::initializer_list<U> ilist,
                                    Args&&... args) {
    // add an element to the back of the queue first
    this->elements.emplace_back();
    new (&this->elements.back().storage)
        Type{ilist, std::forward<Args>(args)...};
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
    return this->open_slots != 0;
}

template <typename Type>
bool Channel<Type>::try_lock_write() {

    auto lck = std::unique_lock<std::mutex>{this->mtx};
    if (this->can_write_proceed()) {
        // release the lock ownership without releasing it, any thread that
        // gets to this point must follow with an immediate write operation
        // and release the lock
        lck.release();
        return true;
    } else {
        return false;
    }
}

template <typename Type>
void Channel<Type>::finish_write(Type value) {

    // adopt the mutex assuming that it is already held
    auto lck = std::unique_lock<std::mutex>{this->mtx, std::adopt_lock};
    assert(this->can_write_proceed());
    this->do_write_no_block([&value, this]() {
        this->enqueue_element(std::move(value));
    });
}

template <typename Type>
void Channel<Type>::notify_waiting_readers() {
    this->read_cv.notify_one();
    for (auto& cv : this->select_cvs_read) {
        cv->notify_one();
    }
}

template <typename Type>
void Channel<Type>::notify_waiting_writers() {
    this->write_cv.notify_one();
    for (auto& cv : this->select_cvs_write) {
        cv->notify_one();
    }
}

template <typename Type>
void Channel<Type>::add_reader_cv(std::shared_ptr<std::condition_variable> cv) {
    this->select_cvs_read.push_back(std::move(cv));
}

template <typename Type>
void Channel<Type>::add_writer_cv(std::shared_ptr<std::condition_variable> cv) {
    this->select_cvs_write.push_back(std::move(cv));
}

namespace detail {

    /**
     * Concepts
     */
    /**
     * Check if the function returns void
     */
    template <typename ChannelType, typename Func>
    using EnableIfCanRead = std::enable_if_t<std::is_same<
        decltype(std::declval<Func>()(
            std::declval<typename std::decay_t<ChannelType>::value_type>())),
        decltype(std::declval<Func>()(
            std::declval<typename std::decay_t<ChannelType>::value_type>()))>
        ::value>;
    template <typename C, typename Func>
    using EnableIfCanWrite = std::enable_if_t<std::is_constructible<
        typename std::decay_t<C>::value_type, decltype(std::declval<Func>()())>
        ::value>;

    /**
     * Detect if the return type of the function is void and return a
     * sharp::Function<bool()> that speculatively tries to read from the
     * channel in the selection context into the lambda passed and returns
     * true if the read was successful, and false otherwise
     */
    template <typename ChannelType, typename Func,
              EnableIfCanRead<ChannelType, Func>* = nullptr>
    sharp::Function<bool()> make_try_func(
            std::shared_ptr<std::condition_variable> cv,
            std::pair<ChannelType&, Func>& context) {

        context.first.add_reader_cv(std::move(cv));
        return [&context]() {

            // try and read
            auto val = context.first.try_read();

            // if the read was successful then return true and call the
            // function, if false then return false
            if (val) {
                context.second(std::move(val.value()));
                return true;
            } else {
                return false;
            }
        };
    }

    /**
     * Detect if the return type of the function is not void, and then in that
     * case return a wrapper that tries to write to the channel and returns
     * true if the write was succesful
     */
    template <typename ChannelType, typename Func,
              EnableIfCanWrite<ChannelType, Func>* = nullptr>
    sharp::Function<bool()> make_try_func(
            std::shared_ptr<std::condition_variable> cv,
            std::pair<ChannelType&, Func>& context) {

        context.first.add_writer_cv(std::move(cv));
        return [&context]() {

            // try and see if the channel is writable
            if (context.first.try_lock_write()) {
                context.first.finish_write(context.second());
                return true;
            } else {
                return false;
            }
        };
    }

    template <typename... SelectContexts>
    std::vector<sharp::Function<bool()>>
    make_try_funcs(std::shared_ptr<std::condition_variable> cv,
                   SelectContexts&... args_in) {

        auto return_vector = std::vector<sharp::Function<bool()>>{};

        // put the args into a tuple and then iterate over it
        auto args = std::forward_as_tuple(args_in...);
        sharp::for_each(args, [&return_vector, &cv](auto& context) {
            return_vector.push_back(make_try_func(cv, context));
        });

        return return_vector;
    }

} // namespace detail

template <typename... SelectContexts>
void channel_select(SelectContexts&&... contexts) {

    // make a vector of functions that can be used to try and see if the read
    // or write has succeeded or not, then this vector will be filled with the
    // compile time generated wrappers from the contexts
    auto cv = std::make_shared<std::condition_variable>();
    auto mtx = std::make_shared<std::mutex>();
    auto try_funcs = detail::make_try_funcs(cv, contexts...);

    // loop while there is nothing to read or write from
    while (true) {
        for (auto& func : try_funcs) {
            if (func()) {
                return;
            }
        }

        // wait till signalled, let spuriosly wakeup, doesn't affect
        // correctness, will change to be more efficient later
        auto lck = std::unique_lock<std::mutex>{*mtx};
        cv->wait(lck);
    }
}

} // namespace sharp
