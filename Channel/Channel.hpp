/**
 * @file Channel.hpp
 * @author Aaryaman Sagar
 *
 * This file contains an implementation for channels in C++, these channels
 * work exactly the same as the channels in the Go language, along with the
 * convenience and multiplexing operations supported by channels
 */

#pragma once

#include <sharp/Tags/Tags.hpp>
#include <sharp/Concurrent/Concurrent.hpp>
#include <sharp/Portability/cpp17.hpp>

#include <mutex>
#include <condition_variable>
#include <utility>
#include <deque>
#include <memory>
#include <exception>
#include <initializer_list>
#include <vector>

namespace sharp {

/**
 * A synchronous channel that can be used for synchronization across multiple
 * threads.  This is an implementation of channels as found in the Go
 * programming langauge with an efford made to maintain the same API with the
 * expressiveness and features of C++
 *
 * A channel can be logically considered to be a n-semaphore, i.e. a semaphore
 * that can at max be incremented up to n values with the n+1th up()
 * opreration blocking until there is space in the semaphore via another
 * down() operation.  In addition channels can be used to send values across
 * along with the gating provided by semaphores.  In other words a channel is
 * an n-bounded threadsafe queue.
 *
 * Send calls on a channel block until either there is enough space in the
 * channel's buffer to accomodate another value or there is a reader waiting
 * on the other end to read the value
 *
 * Channels can also be used to stream data reactively to another thread, with
 * the API being a pair of channel iterators.  This means that the channel can
 * be used in a simple for loop
 *
 *      for (auto value : channel) {
 *          cout << value << endl;
 *      }
 *
 * The loop finishes when the other end calls close() on the channel
 *
 * Something to note here is that the channel is unlike the one in go, it is a
 * non movable non copyable entity, similar to std::mutex and
 * std::condition_variable.  However it is easy to get two seperate threads to
 * operate on the same channel without having to worry that much about
 * lifetime by wrapping the channel in a shared_ptr, for example
 *
 *      auto channel = std::make_shared<Channel<int>>(1);
 *
 *      auto th_one = std::thread{[channel]() {
 *          // this send will not block as the channel has room for one
 *          // element in its internal buffer
 *          channel.send(1);
 *      }};
 *      auto th_two = std::thread{[channel]() {
 *          // this read will block until the channel has a value
 *          cout << channel.read() << endl;
 *      }};
 *
 * Further the behavior of the channel can be customized to fit thread
 * implementations by changing the mutex and condition variable type
 *
 * Channels also capture the value or error semantics of Go channels by
 * providing methods to send exceptions across channels, for example
 *
 *      channel.send_exception(exception_ptr);
 *
 *      try {
 *          channel.read();
 *      } catch(std::exception& exc) {
 *          cerr << exc.what() << endl;
 *      }
 *
 * If exceptions are common and erroneous conditions are to be expected then
 * you can avoid costly rethrows by calling Channel::read_try().  This will
 * return a sharp::Try object.  sharp::Try logically models either a value,
 * an exception or nothing at all, kind of like an optional with another state
 * to store exceptions.  This can therefore be used to manually check for
 * errors.  Hopefully there is a feature in the C++ library to allow
 * visitation on exceptions ith RAII and generalized overloading.  If done,
 * then this can be used to visit exception pointers in an efficient way.  See
 * sharp::visit to get an idea of what this might look like
 *
 *      auto try = channel.read_try();
 *      if (try.has_value()) {
 *          cout << *try << endl;
 *      } else {
 *          cout << "Channel read returned an error" << endl;
 *      }
 *
 * Select statements are also supported, see the documentation above the
 * select() function below for more information about its semantics and usage
 * but for a quick example
 *
 *      sharp::select(
 *          sharp::case(channel_one, []() -> int {
 *              return 1;
 *          }),
 *          sharp::case(channel_two, [](auto value) {
 *              cout << "Read value " << value << endl;
 *          })
 *      );
 *
 * Where a write from the channel is modelled by a callable that returns a
 * value and a read is modeled by a callable that accepts a single value
 */
template <typename Type,
          typename Mutex = std::mutex,
          typename Cv = std::condition_variable>
class Channel {
public:

    /**
     * The constructor of the channel sets the size of the internal buffer
     * that the channel will have.  And then this will affect the semantics of
     * which read and/or write will block and under what conditions
     */
    explicit Channel(int buffer_size = 0);

    /**
     * Channels once created cannot be moved or copied around to other
     * channels.  If the intended use is to have a container of channels, then
     * the way to use a channel with the container would be to wrap it around
     * a wrapper like std::optional, std::unique_ptr or std::shared_ptr and
     * then dump it in the container
     *
     * This behavior is similar to std::mutex, std::condition_variable and
     * std::atomic.  This is a synchronization primitive and should not be
     * moved around
     */
    Channel(const Channel&) = delete;
    Channel(Channel&&) = delete;
    Channel& operator=(const Channel&) = delete;
    Channel& operator=(Channel&&) = delete;

    /**
     * The send function for the channel sends a value across the channel.
     *
     * This blocks until there is space in either the buffer or if there is a
     * reader waiting on the other end waiting to read a value.  This is
     * similar to a (std::future, std::promise) pair but logically different
     * as a future promise pair has a one shot single element queue, whereas a
     * channel has an n-element internal buffer
     *
     * The function has two overloads one for moving values into the channel
     * and another for copying values into the channel.  Both guaranteeing
     * that when the call finishes, the value will either have been moved or
     * copied
     *
     *      // this will move the value across the channel
     *      channel.send(std::move(value));
     *
     *      // this will copy the value into the channel, from where it will
     *      // be moved into the other end that calls read()
     *      channel.send(value)
     *
     * This allows the library to efficiently send value across the channel.
     * There are also in place construction options with std::in_place that
     * will construct the object in the channel in place with the given
     * arguments
     */
    void send(const Type& value);
    void send(Type&& value);

    /**
     * These construct the value in place in the channel, from where they will
     * be moved into th receiving end.  These are disambiguated with the
     * std::in_place tag for clarity.  For example
     *
     *      channel.send(std::in_place, "some string" 1);
     */
    template <typename... Args>
    void send(std::in_place_t, Args&&... args);
    template <typename U, typename... Args>
    void send(std::in_place_t, std::initializer_list<U> il, Args&&... args);

    /**
     * Speculative versions of send, these return a true if the read was
     * successful, false otherwise.  And either move or copy the value out of
     * the reference into the channel
     *
     * If the boolean returned is false then the referred to value will not be
     * changed in the rvalue reference version, it will not be moved out.  If
     * true is returned however it will be moved into the channel and be left
     * in whatever state it's move constructor leaves it in
     */
    bool try_send(const Type& value);
    bool try_send(Type&& value);

    /**
     * The receive function reads a value from the channel
     *
     * This blocks until there is either a value in the channel or if there is
     * a writer waiting on the other end to send a value
     *
     * The object stored internally via a write is then moved from internal
     * storage into the return value of this function.  Which is then itself
     * stored wherever the surrounding code tells it to go
     */
    Type read();

    /**
     * Speculative read from the channel
     *
     * This returns an std::optional<> that contains the value from the
     * channel if a read was successful, otherwise the optional is in an empty
     * state
     *
     * Note that this can throw when the element to be returned is an error
     */
    std::optional<Type> try_read();

    /**
     * Read a try from the channel, this will either represent an exception or
     * a value, see sharp/Try/README.md for more information on Try
     *
     * Note that this function does not throw, any error state is stored in
     * the Try object and can be used in situations where throws are common
     */
    sharp::Try<Type> read_try();

    /**
     * Speculative read of a Try<>, same as try_read() but returns a Try<>
     * which is in an empty state when a read is not possible
     */
    sharp::Try<Type> try_read_try();

    /**
     * Iterator class for the channel
     *
     * This allows iterating through the values of a channel and allows the
     * channel to behave like a synchronous yet to be completed range of
     * values that can be filled asynchronously
     */
    class Iterator;

    /**
     * Begin and end functions to return iterators that can be used to iterate
     * through the values in the channel until the channel is marked closed
     */
    Iterator begin();
    Iterator end();

    /**
     * Close the channel and mark the channel as a completed range, after this
     * point any read will throw an exception and any iteration will stop
     * after reading in all the elements that are currently in the channel
     */
    void close();

    /**
     * Returns true indicating that the channel has been closed, note that
     * this will need to block on an internal mutex to fetch the value of the
     * boolean
     */
    bool is_closed();

    /**
     * Make friends with the select function, each select operation waits on a
     * monitor for the channel to either have a ready value or to be ready for
     * writing
     */
    template <typename... SelectContexts>
    friend void select(SelectContexts&&...);

private:

    template <typename EnqueueFunc>
    void send_impl(EnqueueFunc enqueue);
    template <typename EnqueueFunc>
    bool try_send_impl(EnqueueFunc enqueue);

    /**
     * The number of objects the internal queue can hold without blocking,
     * once this is set at construction time it never changes
     */
    const int buffer_length{0};

    struct State {
        /**
         * The number of open slots in the current buffer, this corresponds to
         * the number of readers waiting + the buffer length
         */
        int open_slots{buffer_length};

        /**
         * The queue of objects or exceptions, represented conveniently using
         * sharp::Try, see sharp/Try/README.md for documentation and usage
         * examples of Try
         *
         * Represented by a concurrent object so protected by a mutex
         */
        std::queue<sharp::Try<Type> elements;
    };
    sharp::Concurrent<State> state;
};

/**
 * The select function to allow multiplexing on I/O through a channel.
 * Based on the type of the function passed in to the function call the
 * select method implicitly decides whether the channel is being used to
 * wait on a read or a write operation, and multiplexes I/O based on that
 */
template <typename... SelectStatements>
void select(SelectStatements&&... statements);

} // namespace sharp

#include <sharp/Channel/Channel.ipp>
