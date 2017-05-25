#pragma once

#include <memory>
#include <utility>
#include <exception>
#include <type_traits>
#include <tuple>
#include <algorithm>
#include <iterator>
#include <cassert>

#include <sharp/Range/Range.hpp>
#include <sharp/Traits/Traits.hpp>
#include <sharp/Defer/Defer.hpp>
#include <sharp/Future/Future.hpp>
#include <sharp/Future/FutureError.hpp>
#include <sharp/Future/Promise.hpp>
#include <sharp/Future/detail/FutureImpl.hpp>
#include <sharp/Future/Executor.hpp>

namespace sharp {

namespace detail {

    /**
     * Concepts
     */
    template <typename Type>
    using EnableIfNotFutureType = std::enable_if_t<
        !sharp::IsInstantiationOf_v<Type, sharp::Future>
        && !sharp::IsInstantiationOf_v<Type, sharp::SharedFuture>>;

    /**
     * The when_*** implementation, checks when the iteration has been
     * finished by querying the number of finished objects against func, if
     * func(number_of_finished) returns true then the value in the promise is
     * set
     */
    template <typename Func, typename... Futures>
    auto when_impl_variadic(Func func, Futures&&... futures);

    /**
     * The when_*** runtime implementation, checks when the iteration has been
     * finished by querying the number of finished objects against func, if
     * func(number_of_finished) returns true then the value in the promise is
     * set
     */
    template <typename Func, typename BeginIterator, typename EndIterator>
    auto when_impl_iter(Func func, BeginIterator first, EndIterator last);

    template <typename Type>
    void move_from_promise(Promise<Type>& promise, Future<Type>& future) {
        future = promise.get_future();
    }

} // namespace detail

template <typename Type>
Future<Type>::Future() noexcept {}

template <typename Type>
Future<Type>::Future(const std::shared_ptr<detail::FutureImpl<Type>>& state)
        : shared_state{state} {
    this->shared_state->test_and_set_retrieved_flag();
}

template <typename Type>
Future<Type>::Future(Future&& other) noexcept
        : shared_state{std::move(other.shared_state)} {}

template <typename Type>
Future<Type>::Future(Future<Future<Type>>&& other) : Future{} {

    // check if other has shared state and if it does not then throw an
    // exception
    other.check_shared_state();

    // create a promise for *this and assign the resulting future to *this,
    // creating a shared_ptr to a promise because the add_callback function
    // requires a lambda that is copyable, and this is because it stores the
    // lambda in a std::function object
    auto promise = sharp::Promise<Type>();
    *this = promise.get_future();

    // clear the shared state of the other future when this function exits
    auto deferred = defer_guard([&other]() {
        other.shared_state.reset();
    });

    // add a continuation to other that will be fired when the other future
    // has been fulfilled with a Future<Type> object
    other.shared_state->add_callback([promise = std::move(promise)]
            (auto& shared_state_outer) mutable {

        // store an exception in the current future if either the inner future
        // is not ready or if there is an exception instead of a future in the
        // outer future
        if (shared_state_outer.contains_exception()) {
            promise.set_exception(shared_state_outer.get_exception_ptr());
            return;
        }
        if (!shared_state_outer.get_value().valid()) {
            auto exc = FutureError{FutureErrorCode::broken_promise};
            promise.set_exception(std::make_exception_ptr(exc));
            return;
        }

        // when the outer future has been fulfilled add a continuation to the
        // inner future that will fire when the inner future has been
        // completed, this will then fulfill *this with the value returned by
        // calling get() on the resulting future
        assert(shared_state_outer.get_value().shared_state);
        shared_state_outer.get_value().shared_state->add_callback(
                [promise = std::move(promise)]
                (auto& shared_state_inner) mutable {
            if (shared_state_inner.contains_exception()) {
                promise.set_exception(shared_state_inner.get_exception_ptr());
            } else {
                promise.set_value(shared_state_inner.get());
            }
        });
    });
}

template <typename Type>
Future<Type>& Future<Type>::operator=(Future&& other) noexcept {
    this->shared_state = std::move(other.shared_state);
    return *this;
}

template <typename Type>
Future<Type>::~Future() {}

template <typename Type>
bool Future<Type>::valid() const noexcept {
    // if the future contains a reference count to the shared state then the
    // future is valid
    return static_cast<bool>(this->shared_state);
}

template <typename Type>
void Future<Type>::wait() const {
    this->check_shared_state();
    this->shared_state->wait();
}

template <typename Type>
Type Future<Type>::get() {

    this->check_shared_state();

    // reset the shared state after a successful get(), which will either
    // throw or return the value.  In both cases reset the value
    auto deferred = defer_guard([this]() {
        this->shared_state.reset();
    });
    this->shared_state->wait();
    return this->shared_state->get();
}

template <typename Type>
bool Future<Type>::is_ready() const {
    this->check_shared_state();
    return this->shared_state->is_ready();
}

template <typename Type>
template <typename Func,
          typename detail::EnableIfDoesNotReturnFuture<Func, Type>*>
auto Future<Type>::then(Func&& func)
        -> Future<decltype(func(std::move(*this)))> {
    auto deferred = defer_guard([this]() {
        this->shared_state.reset();
    });
    return detail::ComposableFuture<Future<Type>>::then(
            std::forward<Func>(func));
}

template <typename Type>
template <typename Func,
          typename detail::EnableIfReturnsFuture<Func, Type>*>
auto Future<Type>::then(Func&& func) -> decltype(func(std::move(*this))) {

    auto deferred = defer_guard([this]() {
        this->shared_state.reset();
    });

    // unwrap the returned future
    using T
        = typename std::decay_t<decltype(func(std::move(*this)))>::value_type;
    return Future<T>{
        detail::ComposableFuture<Future<Type>>::then(std::forward<Func>(func))};
}

template <typename... Futures>
auto when_all(Futures&&... args) {
    // make the function that checks when the futures have been completed
    int length = std::tuple_size<decltype(std::make_tuple(
                    sharp::move_if_movable(args)...))>::value;
    auto done = [length](int number_done) {
        assert(number_done <= length);
        return (number_done == length);
    };
    return detail::when_impl_variadic(done, std::forward<Futures>(args)...);
}

template <typename BeginIterator, typename EndIterator,
          detail::EnableIfNotFutureType<BeginIterator>* = nullptr,
          detail::EnableIfNotFutureType<BeginIterator>* = nullptr>
auto when_all(BeginIterator first, EndIterator last) {
    int length = std::distance(first, last);
    auto done = [length](int number_done) {
        assert(number_done <= length);
        return (number_done == length);
    };
    return detail::when_impl_iter(done, first, last);
}

template <typename... Futures>
auto when_any(Futures&&... futures) {
    auto done = [](int number_done) {
        return number_done;
    };
    return detail::when_impl_variadic(done, std::forward<Futures>(futures)...);
}

template <typename BeginIterator, typename EndIterator,
          detail::EnableIfNotFutureType<BeginIterator>* = nullptr,
          detail::EnableIfNotFutureType<BeginIterator>* = nullptr>
auto when_any(BeginIterator first, EndIterator last) {
    auto done = [](int number_done) {
        return number_done;
    };
    return detail::when_impl_iter(done, first, last);
}

namespace detail {

    template <typename FutureType>
    template <typename Func>
    auto ComposableFuture<FutureType>::then(Func&& func)
            -> Future<decltype(func(std::declval<FutureType>()))> {

        this->this_instance().check_shared_state();

        // make a future promise pair, the value returned will be the future
        // from this pair, upon successful completion the future will be
        // satisfied by a callback that is decorated around the one passed in,
        // the decorated callback will capture the shared state of the current
        // future, and then call the callback and pass it a future that is
        // constructed with that shared state the value that the inner
        // callback returns will then be moved into the promise
        auto promise = Promise<decltype(func(std::declval<FutureType>()))>{};
        auto future = std::decay_t<decltype(promise.get_future())>{};
        future.shared_state = std::move(promise.get_future().shared_state);

        this->this_instance().shared_state->add_callback(
                [promise = std::move(promise),
                 func = std::forward<Func>(func),
                 shared_state = this->this_instance().shared_state]
                (auto&) mutable {
            // bypass the normal execution and assign the shared pointer
            // directly
            auto fut = FutureType{};
            fut.shared_state = std::move(shared_state);

            // try and get the value from the callback, if an exception was
            // thrown, propagate that
            try {
                auto val = func(sharp::move_if_movable(fut));
                promise.set_value(std::move(val));
            } catch (...) {
                promise.set_exception(std::current_exception());
                return;
            }
        });

        return future;
    }


    // helper trait
    template <typename F>
    struct PromiseFor {
        using type = sharp::Promise<typename std::decay_t<F>::value_type>;
    };

    template <typename Func, typename... Futures>
    auto when_impl_variadic(Func f, Futures&&... args) {

        // the returning future will be instantiated with this return type
        using ReturnFutures = std::tuple<std::decay_t<Futures>...>;
        using ReturnPromises = sharp::Transform_t<PromiseFor, ReturnFutures>;

        // A single struct to avoid making 2 shared pointers instead of 1
        struct Bookkeeping {
            sharp::Promise<ReturnFutures> promise;
            ReturnFutures return_futures;
            ReturnPromises return_promises;
            std::atomic<int> counter{0};
            Bookkeeping() {
                // set each future to be from the corresponding promise
                sharp::for_each(this->return_futures, [this]
                        (auto& future, auto index) {
                    move_from_promise(
                        std::get<static_cast<int>(index)>(
                            this->return_promises),
                        future);
                });
            }
        };

        // construct the return type from the argument type list, then construct
        // an object that will contain the ready futures when they are all done
        auto futures = std::make_tuple(sharp::move_if_movable(args)...);
        auto bookkeeping = std::make_shared<Bookkeeping>();
        auto future = bookkeeping->promise.get_future();

        // iterate through all the futures and signal the promsise when all of
        // them have been satisfied
        sharp::for_each(futures, [&bookkeeping, &f](auto& future, auto index) {
            // when the future is complete move it into the tuple of ready
            // futures, which will at the end contain all the futures that are
            // ready at a stage where you can signal to the user
            future.then([bookkeeping, index, f](auto future) {
                std::get<static_cast<int>(index)>(bookkeeping->return_promises)
                    .set_value(future.get());

                // if all the futures have been completed, signal
                ++bookkeeping->counter;
                if (f(bookkeeping->counter.load())) {
                    bookkeeping->promise.set_value(
                        std::move(bookkeeping->return_futures));
                }

                // return to make the future code not error out
                return 0;
            });
        });

        return future;
    }

    template <typename Func, typename BeginIterator, typename EndIterator>
    auto when_impl_iter(Func f, BeginIterator first, EndIterator last) {

        // the type of the future that is to be returned
        using FutureValueType = typename std::decay_t<decltype(*first)>
            ::value_type;
        using ReturnFutures = std::vector<std::decay_t<decltype(*first)>>;
        using ReturnPromises = std::vector<sharp::Promise<FutureValueType>>;

        // The bookkeeping shared struct, this will encapsulate three other
        // things, that help in both hitting the cache more and saving on heap
        // allocations via shared pointers
        struct Bookkeeping {
            sharp::Promise<ReturnFutures> promise;
            ReturnFutures return_futures;
            ReturnPromises return_promises;
            std::atomic<int> counter{0};
            Bookkeeping(int length) {
                this->return_promises.reserve(length);
                this->return_futures.reserve(length);
                for (auto i : sharp::range(0, length)) {
                    this->return_promises.emplace_back();
                    this->return_futures.emplace_back();
                    move_from_promise(this->return_promises[i],
                            this->return_futures[i]);
                }
            }
        };

        // make the bookkeeping objects and make the object to return
        auto length = std::distance(first, last);
        auto bookkeeping = std::make_shared<Bookkeeping>(length);
        auto future = bookkeeping->promise.get_future();

        // iterate through all the futures in the bookkeeping struct and then
        // set callbacks on them via .then(), when sufficient futures have
        // finished, signal via the promise that the task has been done
        sharp::for_each(sharp::range(first, last),
        [&bookkeeping, &f](auto& future, auto index) {
            future.then([bookkeeping, index, f](auto future) {
                bookkeeping->return_promises[static_cast<int>(index)]
                    .set_value(future.get());

                ++bookkeeping->counter;
                if (f(bookkeeping->counter.load())) {
                    bookkeeping->promise.set_value(
                        std::move(bookkeeping->return_futures));
                }

                return 0;
            });
        });

        return future;
    }

} // namespace detail

template <typename Type>
void Future<Type>::check_shared_state() const {
    if (!this->valid()) {
        throw FutureError{FutureErrorCode::no_state};
    }
}

} // namespace sharp

#include <sharp/Future/SharedFuture.hpp>
