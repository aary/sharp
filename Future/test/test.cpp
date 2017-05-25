#include <thread>
#include <chrono>
#include <utility>
#include <iostream>
#include <vector>

#include <sharp/Future/Future.hpp>
#include <sharp/Threads/Threads.hpp>
#include <gtest/gtest.h>

TEST(Future, FutureBasic) {
    auto promise = sharp::Promise<int>{};
    auto future = promise.get_future();
    promise.set_value(1);
    auto value = future.get();
    EXPECT_EQ(value, 1);
}

TEST(Future, FutureBasicThreaded) {
    auto promise = sharp::Promise<int>{};
    auto future = promise.get_future();
    auto th = std::thread{[&]() {
        promise.set_value(10);
    }};
    EXPECT_EQ(future.get(), 10);
    th.join();
}

TEST(Future, FutureMove) {
    auto promise = sharp::Promise<int>{};
    auto future = promise.get_future();
    EXPECT_TRUE(future.valid());
    auto another_future = std::move(future);
    EXPECT_TRUE(another_future.valid());
    EXPECT_FALSE(future.valid());

    future = std::move(another_future);
    EXPECT_TRUE(future.valid());
    EXPECT_FALSE(another_future.valid());
}

TEST(Future, FutureInvalid) {
    auto promise = sharp::Promise<int>{};
    auto future = promise.get_future();
    promise.set_value(1);
    future.get();
    try {
        future.is_ready();
        EXPECT_TRUE(false);
    } catch (...) {}
}

TEST(Future, FutureExceptionSend) {
    auto promise = sharp::Promise<int>{};
    auto future = promise.get_future();
    promise.set_exception(std::make_exception_ptr(std::logic_error{""}));
    try {
        future.get();
        EXPECT_TRUE(false);
    } catch (std::logic_error& err) {
    } catch (...) {
        EXPECT_TRUE(false);
    }
}

TEST(Future, FutureAlreadyRetrieved) {
    auto promise = sharp::Promise<int>{};
    auto future = promise.get_future();
    try {
        auto future = promise.get_future();
        EXPECT_TRUE(false);
    } catch (sharp::FutureError& err) {
        EXPECT_EQ(err.code().value(), static_cast<int>(
                    sharp::FutureErrorCode::future_already_retrieved));
    }
}

TEST(Future, PromiseAlreadySatisfied) {
    auto promise = sharp::Promise<int>{};
    promise.set_value(1);
    try {
        promise.set_value(1);
        EXPECT_TRUE(false);
    } catch(sharp::FutureError& err) {
        EXPECT_EQ(err.code().value(), static_cast<int>(
                    sharp::FutureErrorCode::promise_already_satisfied));
    }
}

TEST(Future, NoState) {
    auto future = sharp::Future<int>{};

    try {
        future.get();
        EXPECT_TRUE(false);
    } catch(sharp::FutureError& err) {
        EXPECT_EQ(err.code().value(), static_cast<int>(
                    sharp::FutureErrorCode::no_state));
    }

    try {
        future.wait();
        EXPECT_TRUE(false);
    } catch(sharp::FutureError& err) {
        EXPECT_EQ(err.code().value(), static_cast<int>(
                    sharp::FutureErrorCode::no_state));
    }

}

TEST(Future, DoubleGet) {
    auto promise = sharp::Promise<int>{};
    auto future = promise.get_future();
    promise.set_value(1);
    future.get();
    try {
        future.get();
        EXPECT_TRUE(false);
    } catch(sharp::FutureError& err) {
        EXPECT_EQ(err.code().value(), static_cast<int>(
                    sharp::FutureErrorCode::no_state));
    }
}

TEST(Future, BrokenPromise) {
    auto future = sharp::Future<int>{};
    {
        auto promise = sharp::Promise<int>{};
        auto future_two = promise.get_future();
        future = std::move(future_two);
    }
    try {
        future.get();
        EXPECT_TRUE(false);
    } catch (sharp::FutureError& err) {
        EXPECT_EQ(err.code().value(), static_cast<int>(
                    sharp::FutureErrorCode::broken_promise));
    }
}

TEST(Future, UnwrapConstructBasic) {
    for (auto i = 0; i < 1000; ++i) {
        sharp::ThreadTest::reset();
        auto promise = sharp::Promise<sharp::Future<int>>{};
        auto future_unwrapped = sharp::Future<int>{promise.get_future()};

        std::thread{[promise = std::move(promise)]() mutable {
            sharp::ThreadTest::mark(1);
            auto promise_inner = sharp::Promise<int>{};
            auto future_inner = promise_inner.get_future();
            promise.set_value(std::move(future_inner));
            promise_inner.set_value(1);
        }}.detach();

        sharp::ThreadTest::mark(0);
        EXPECT_EQ(future_unwrapped.get(), 1);
    }
}

TEST(Future, UnwrapConstructOtherInvalid) {
    try {
        auto future = sharp::Future<sharp::Future<int>>{};
        auto future_unwrapped = sharp::Future<int>{std::move(future)};
        EXPECT_TRUE(false);
    } catch (sharp::FutureError& err) {
        EXPECT_EQ(err.code().value(), static_cast<int>(
                    sharp::FutureErrorCode::no_state));
    }
}

TEST(Future, UnwrapConstructOtherContainsException) {
    try {
        auto promise = sharp::Promise<sharp::Future<int>>{};
        auto future = promise.get_future();
        auto future_unwrapped = sharp::Future<int>{std::move(future)};
        promise.set_exception(std::make_exception_ptr(std::logic_error{""}));
        future_unwrapped.get();
        EXPECT_TRUE(false);
    } catch (std::logic_error& err) {}
}

TEST(Future, UnwrapConstructOtherContainsInvalid) {
    try {
        auto promise = sharp::Promise<sharp::Future<int>>{};
        auto future = promise.get_future();
        auto future_unwrapped = sharp::Future<int>{std::move(future)};
        promise.set_value(sharp::Future<int>{});
        future_unwrapped.get();
        EXPECT_TRUE(false);
    } catch (sharp::FutureError& err) {
        EXPECT_EQ(err.code().value(), static_cast<int>(
                    sharp::FutureErrorCode::broken_promise));
    }
}

TEST(Future, UnwrapConstructOtherContainsValidWithException) {
    auto promise = sharp::Promise<sharp::Future<int>>{};
    auto future = promise.get_future();
    auto promise_inner = sharp::Promise<int>{};
    auto future_inner = promise_inner.get_future();
    auto future_unwrapped = sharp::Future<int>{std::move(future)};

    try {
        promise_inner.set_exception(
                std::make_exception_ptr(std::logic_error{""}));
        promise.set_value(std::move(future_inner));
        future_unwrapped.get();
        EXPECT_TRUE(false);
    } catch (std::logic_error& err) {}
}

TEST(Future, FutureThenBasicTest) {
    auto promise = sharp::Promise<int>{};
    auto future = promise.get_future();
    auto thened_future = future.then([](auto future) {
        return future.get() * 5;
    });
    EXPECT_FALSE(future.valid());
    promise.set_value(10);
    EXPECT_EQ(thened_future.get(), 50);
}

TEST(Future, FutureThenThreaded) {
    for (auto i = 0; i < 100; ++i) {
        auto promise = sharp::Promise<int>{};
        auto future = promise.get_future();
        std::thread{[&]() {
            promise.set_value(10);
        }}.detach();
        auto thened_future = future.then([](auto future) {
            return future.get() * 5;
        });
        EXPECT_EQ(thened_future.get(), 50);
    }
}

TEST(Future, FutureThenException) {
    for (auto i = 0; i < 100; ++i) {
        auto promise = sharp::Promise<int>{};
        auto future = promise.get_future();
        auto another_future = future.then([](auto) {
            throw std::runtime_error{""};
            return 1;
        });

        std::thread{[promise = std::move(promise)]() mutable {
            promise.set_value(3);
        }}.detach();

        try {
            another_future.get();
            EXPECT_TRUE(false);
        } catch (std::runtime_error&) {}
    }
}

TEST(Future, FutureThenExceptionIndirection) {
    for (auto i = 0; i < 100; ++i) {
        auto promise = sharp::Promise<int>{};
        auto future = promise.get_future();
        auto another_future = future.then([](auto future) {
            return future.get() * 10;
        });

        std::thread{[promise = std::move(promise)]() mutable {
            promise.set_exception(
                    std::make_exception_ptr(std::runtime_error{""}));
        }}.detach();

        try {
            another_future.get();
            EXPECT_TRUE(false);
        } catch (std::runtime_error&) {}
    }
}

TEST(Future, FutureThenExceptionTwoLevel) {
    for (auto i = 0; i < 100; ++i) {
        auto promise = sharp::Promise<int>{};
        auto future = promise.get_future();

        auto counter = 0;
        auto another_future = future.then([&counter](auto future) {
            EXPECT_EQ(counter++, 0);
            throw std::runtime_error{""};
            return future.get();
        }).then([&counter](auto future) {
            EXPECT_EQ(counter++, 1);
            return future.get() * 2;
        });

        std::thread{[promise = std::move(promise)]() mutable {
            promise.set_value(2);
        }}.detach();

        try {
            another_future.get();
            EXPECT_TRUE(false);
        } catch (std::runtime_error&) {}
    }
}

TEST(Future, FutureThenExceptionTwoLevelSecondThrows) {
    for (auto i = 0; i < 100; ++i) {
        auto promise = sharp::Promise<int>{};
        auto future = promise.get_future();

        auto counter = 0;
        auto another_future = future.then([&counter](auto future) {
            EXPECT_EQ(counter++, 0);
            return future.get();
        }).then([&counter](auto future) {
            EXPECT_EQ(counter++, 1);
            throw std::runtime_error{""};
            return future.get() * 2;
        });

        std::thread{[promise = std::move(promise)]() mutable {
            promise.set_value(2);
        }}.detach();

        try {
            another_future.get();
            EXPECT_TRUE(false);
        } catch (std::runtime_error&) {}
    }
}

TEST(Future, FutureThenExceptionIndirectionTwoLevel) {
    for (auto i = 0; i < 100; ++i) {
        auto promise = sharp::Promise<int>{};
        auto future = promise.get_future();

        auto counter = 0;
        auto another_future = future.then([&counter](auto future) {
            EXPECT_EQ(counter++, 0);
            return future.get() * 2;
        }).then([&counter](auto future) {
            EXPECT_EQ(counter++, 1);
            return future.get() * 2;
        });

        std::thread{[promise = std::move(promise)]() mutable {
            promise.set_exception(
                    std::make_exception_ptr(std::runtime_error{""}));
        }}.detach();

        try {
            another_future.get();
            EXPECT_TRUE(false);
        } catch (std::runtime_error&) {}
    }
}

TEST(Future, FutureThenMultipleThensValuePropagate) {
    for (auto i = 0; i < 100; ++i) {
        auto promise = sharp::Promise<int>{};
        auto future = promise.get_future();

        auto counter = 0;
        auto another_future = future.then([&counter](auto future) {
            EXPECT_EQ(counter++, 0);
            return future.get() * 2;
        }).then([&counter](auto future) {
            EXPECT_EQ(counter++, 1);
            return future.get() * 2;
        }).then([&counter](auto future) {
            EXPECT_EQ(counter++, 2);
            return future.get() * 2;
        }).then([&counter](auto future) {
            EXPECT_EQ(counter++, 3);
            return future.get() * 2;
        });

        std::thread{[promise = std::move(promise)]() mutable {
            promise.set_value(1);
        }}.detach();

        EXPECT_EQ(another_future.get(), 16);
    }
}

TEST(Future, FutureThenMultipleThensValueUnwrappedPropagate) {
    for (auto i = 0; i < 100; ++i) {
        auto promise = sharp::Promise<int>{};
        auto future = promise.get_future();

        auto counter = std::make_shared<int>();
        auto another_future = future.then([counter](auto future) {
            EXPECT_EQ((*counter)++, 0);
            auto value = future.get();
            auto promise = sharp::Promise<int>{};
            auto future_other = promise.get_future();
            std::thread{[value, promise = std::move(promise)]() mutable {
                promise.set_value(value * 2);
            }}.detach();
            return future_other;
        }).then([counter](auto future) {
            EXPECT_EQ((*counter)++, 1);
            auto value = future.get();
            auto promise = sharp::Promise<int>{};
            auto future_other = promise.get_future();
            std::thread{[value, promise = std::move(promise)]() mutable {
                promise.set_value(value * 2);
            }}.detach();
            return future_other;
        }).then([counter](auto future) {
            EXPECT_EQ((*counter)++, 2);
            auto value = future.get();
            auto promise = sharp::Promise<int>{};
            auto future_other = promise.get_future();
            std::thread{[value, promise = std::move(promise)]() mutable {
                promise.set_value(value * 2);
            }}.detach();
            return future_other;
        }).then([counter](auto future) {
            EXPECT_EQ((*counter)++, 3);
            auto value = future.get();
            auto promise = sharp::Promise<int>{};
            auto future_other = promise.get_future();
            std::thread{[value, promise = std::move(promise)]() mutable {
                promise.set_value(value * 2);
            }}.detach();
            return future_other;
        });

        std::thread{[promise = std::move(promise)]() mutable {
            promise.set_value(1);
        }}.detach();

        EXPECT_EQ(another_future.get(), 16);
    }
}

TEST(Future, FutureWhenAllBasic) {
    for (auto i = 0; i < 100; ++i) {
        auto promise_one = sharp::Promise<int>{};
        auto future_one = promise_one.get_future();
        auto promise_two = sharp::Promise<int>{};
        auto future_two = promise_two.get_future();
        auto promise_three = sharp::Promise<int>{};
        auto future_three = promise_three.get_future();

        auto future = sharp::when_all(future_one, future_two, future_three);

        EXPECT_FALSE(future_one.valid() || future_two.valid()
                || future_three.valid());

        std::thread{[promise = std::move(promise_one)]() mutable {
            promise.set_value(1);
        }}.detach();
        std::thread{[promise = std::move(promise_two)]() mutable {
            promise.set_value(2);
        }}.detach();
        std::thread{[promise = std::move(promise_three)]() mutable {
            promise.set_value(3);
        }}.detach();

        auto tuple_futures = future.get();
        EXPECT_EQ(std::get<0>(tuple_futures).get(), 1);
        EXPECT_EQ(std::get<1>(tuple_futures).get(), 2);
        EXPECT_EQ(std::get<2>(tuple_futures).get(), 3);
    }
}

TEST(Future, FutureWhenAllRuntimeBasic) {
    for (auto i = 0; i < 100; ++i) {
        auto promise_one = sharp::Promise<int>{};
        auto future_one = promise_one.get_future();
        auto promise_two = sharp::Promise<int>{};
        auto future_two = promise_two.get_future();
        auto promise_three = sharp::Promise<int>{};
        auto future_three = promise_three.get_future();

        auto futures = std::vector<sharp::Future<int>>{};
        futures.push_back(std::move(future_one));
        futures.push_back(std::move(future_two));
        futures.push_back(std::move(future_three));

        auto future = sharp::when_all(futures.begin(), futures.end());

        EXPECT_FALSE(futures[0].valid() || futures[1].valid()
                || futures[2].valid());

        std::thread{[promise = std::move(promise_one)]() mutable {
            promise.set_value(1);
        }}.detach();
        std::thread{[promise = std::move(promise_two)]() mutable {
            promise.set_value(2);
        }}.detach();
        std::thread{[promise = std::move(promise_three)]() mutable {
            promise.set_value(3);
        }}.detach();

        auto vector_futures = future.get();
        EXPECT_EQ(vector_futures[0].get(), 1);
        EXPECT_EQ(vector_futures[1].get(), 2);
        EXPECT_EQ(vector_futures[2].get(), 3);
    }
}

TEST(Future, FutureWhenAnyBasic) {
    for (auto i = 0; i < 100; ++i) {
        auto promise_one = sharp::Promise<int>{};
        auto future_one = promise_one.get_future();
        auto promise_two = sharp::Promise<int>{};
        auto future_two = promise_two.get_future();
        auto promise_three = sharp::Promise<int>{};
        auto future_three = promise_three.get_future();

        auto future = sharp::when_any(future_one, future_two, future_three);

        EXPECT_FALSE(future_one.valid() || future_two.valid()
                || future_three.valid());

        std::thread{[promise = std::move(promise_two)]() mutable {
            promise.set_value(2);
        }}.detach();

        auto tuple_futures = future.get();
        EXPECT_TRUE(std::get<0>(tuple_futures).valid());
        EXPECT_TRUE(std::get<1>(tuple_futures).valid());
        EXPECT_TRUE(std::get<2>(tuple_futures).valid());

        EXPECT_EQ(std::get<1>(tuple_futures).get(), 2);
        EXPECT_FALSE(std::get<0>(tuple_futures).is_ready());
        EXPECT_FALSE(std::get<2>(tuple_futures).is_ready());
    }
}

TEST(Future, SharedFutureBasic) {
    auto promise = sharp::Promise<int>{};
    auto future = promise.get_future().share();
    promise.set_value(1);
    for (auto i = 0; i < 10; ++i) {
        EXPECT_EQ(future.get(), 1);
    }
}

TEST(Future, SharedFutureThen) {
    for (auto i = 0; i < 100; ++i) {
        auto promise = sharp::Promise<int>{};
        auto future = promise.get_future();
        auto shared_future = future.share();

        auto future_after = shared_future.then([](auto s_future) {
            auto promise = sharp::Promise<int>{};
            auto future = promise.get_future();
            std::thread{[s_future, promise = std::move(promise)]() mutable {
                promise.set_value(s_future.get() * 2);
            }}.detach();
            return future;
        });

        promise.set_value(3);
        EXPECT_EQ(future_after.get(), 6);
    }
}

TEST(Future, SharedFutureWhenAll) {
    for (auto i = 0; i < 100; ++i) {
        auto promise_one = sharp::Promise<int>{};
        auto future = promise_one.get_future();
        auto promise_two = sharp::Promise<int>{};
        auto future_shared = promise_two.get_future().share();

        auto thenned = sharp::when_all(future, future_shared).then(
                [](auto futures) {
            auto tuple_futures = futures.get();
            return std::get<0>(tuple_futures).get()
                * std::get<1>(tuple_futures).get();
        });

        std::thread{[promise = std::move(promise_one)]() mutable {
            promise.set_value(4);
        }}.detach();
        std::thread{[promise = std::move(promise_two)]() mutable {
            promise.set_value(3);
        }}.detach();

        EXPECT_EQ(thenned.get(), 12);
    }
}

TEST(Future, FutureGetSetSpeedTest) {

    const auto LIMIT = 100000;

    // create a bunch of futures and promises
    auto promises = std::vector<sharp::Promise<int>>{};
    auto futures = std::vector<sharp::Future<int>>{};
    for (auto i = 0; i < LIMIT; ++i) {
        promises.emplace_back();
        futures.push_back(promises[i].get_future());
    }

    // then create two threads to set values in even futures and the other to
    // set values in the odd futures
    std::thread{[&promises]() {
        for (auto i = 0; i < LIMIT; i += 2) {
            promises[i].set_value(i);
        }
    }}.detach();
    std::thread{[&promises]() {
        for (auto i = 1; i < LIMIT; i += 2) {
            promises[i].set_value(i);
        }
    }}.detach();

    // then create two threads that get() values from even and odd futures
    auto th_one = std::thread{[&futures]() {
        for (auto i = 0; i < LIMIT; i += 2) {
            futures[i].get();
        }
    }};
    auto th_two = std::thread{[&futures]() {
        for (auto i = 1; i < LIMIT; i += 2) {
            futures[i].get();
        }
    }};

    th_one.join();
    th_two.join();

}

TEST(Future, FutureThenSpeedTest) {

    // changing this to 100000 makes the program exit with an error, so what I
    // have done is break the loop into two parts, one outer loop and one
    // inner loop that does the stess execution, the breakage means that the
    // inner loop will release the memory it had previously acquired
    const auto LIMIT = 100000;
    const auto DIVIDER = 10;

    for (auto outer = 0; outer < DIVIDER; ++outer) {

        auto promise = sharp::Promise<int>{};
        auto future = promise.get_future();

        auto counter = std::make_shared<int>();
        for (auto i = 0; i < LIMIT/DIVIDER; ++i) {
            future = future.then([counter](auto) {
                (*counter)++;
                return 1;
            });
        }
        EXPECT_EQ((*counter), 0);
        promise.set_value(1);
        EXPECT_EQ((*counter), LIMIT / DIVIDER);
        EXPECT_EQ(future.get(), 1);
        EXPECT_EQ((*counter), LIMIT / DIVIDER);
    }
}
