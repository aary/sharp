/*
 * @file Channel.hpp
 * @author Aaryaman Sagar
 *
 * This file contains an implementation for channels in C++, these channels
 * work exactly the same as the channels in the Go language, along with the
 * convenience and multiplexing operations supported by channels in the go
 * language
 */

#pragma once

#include <mutex>
#include <condition_variable>
#include <utility>

/**
 * An asynchronous channel that can be used for synchronization across
 * multiple threads.  This is an implementation of channels as found in the Go
 * language, with an effort made to maintain the same API as much as possible
 * without losing the expressiveness of the original API.
 *
 * A channel is a logical synchronization utility that can be used like
 * generalized semaphores, in fact a buffered Channel is the same as a
 * generalized semaphore (except for the upper bound on the number of times
 * values can be added without fetching, which seems to be designed to prevent
 * user errors)
 *
 * Send and receive calls block on the channel if the buffer cannot handle
 * another value, the value blocking on will be released as soon as a reader
 * fetches the value with a read() call
 */
template <typename Type>
class Channel {
public:

    /**
     * The send function to send a value across the channel
     *
     * Unlike socket asycnhronous sends, sends on a channel block until they
     * are received on the other end, resulting in a synchronization pattern
     * like with futures, but slightly different since there can be multiple
     * sends on a single channel
     *
     * The type is perfectly forwarded and an object is constructed within the
     * channel to allow the other end to fetch the object.  Whether or not
     * this is bad for cache coherency is yet to be determined, but for now
     * moving values into the channel is acceptable
     */
    void send(const Type& object);
    void send(Type&& object);

    /**
     * The receive function to receive a value from the channel
     *
     * This call is blocking, it will block until there is a value to be read
     * from the channel, thus this is very similar to a fetch on a future
     *
     * The object that was stored internally in the socket is moved into the
     * receiving code when this is called, as a result there may be a return
     * value optimization into the stored object
     */
    Type read();

    /**
     * Constructor for the channel accepts a buffer length argument that can
     * be used to set the initial capacity for the channel, the default
     * capacity of 0 implies that the channel is not buffered and every send
     * must be received before other sends can be received.
     */
    Channel(int buffer_length = 0);

    /**
     * An iterator class for the channel to represent a range within the
     * channel.  This enables continuous streaming of objects from one thread
     * to another with a simple generalized reading approach.  See README for
     * the module for more information.
     */
    class Iterator;

    /**
     * Begin and end iterators to represent a range within the channel.  This
     * enables continuous streaming of objects from one thread to another with
     * a simple generalized reading approach.  See README for the module for
     * more information.
     */
    Iterator begin();
    Iterator end();

    /**
     * The select function to allow multiplexing on I/O through a channel.
     * Based on the type of the function passed in to the function call the
     * select method implicitly decides whether the channel is being used to
     * wait on a read or a write operation, and multiplexes I/O based on that
     */
    template <typename Func, typename... Args>
    static void select(std::pair<Channel&, Function>, Args&&...);
};
