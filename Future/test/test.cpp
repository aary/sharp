#include <thread>
#include <chrono>
#include <utility>

#include <sharp/Future/Future.hpp>
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
