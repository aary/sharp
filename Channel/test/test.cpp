#include <thread>
#include <vector>
#include <random>
#include <iostream>

#include <gtest/gtest.h>
#include <sharp/Channel/Channel.hpp>

TEST(Channel, BasicTest) {
    sharp::Channel<int> ch{1};
    ch.send(1);
    EXPECT_EQ(ch.read(), 1);
}

TEST(Channel, TryReadFail) {
    sharp::Channel<int> ch_one;
    EXPECT_FALSE(ch_one.try_read());

    sharp::Channel<int> ch_two{2};
    EXPECT_FALSE(ch_two.try_read());
}

TEST(Channel, SendTwoValues) {
    sharp::Channel<int> c{2};
    c.send(2);
    c.send(3);
    EXPECT_EQ(c.read(), 2);
    EXPECT_EQ(c.read(), 3);
    EXPECT_FALSE(c.try_read());
    auto th = std::thread{[&]() {
        EXPECT_EQ(c.read(), 4);
    }};
    c.send(4);
    th.join();
}

TEST(Channel, UnbufferedThreadedSend) {
    sharp::Channel<int> c;
    auto number_iterations = 1e4;

    // the results vector, although this is not locked with a mutex, access to
    // this is atomic since the read thread only reads a value after it has
    // been written to
    auto results = std::vector<int>{};
    results.resize(number_iterations);

    // the random number generator
    auto rng = std::mt19937{};
    rng.seed(std::random_device()());
    auto dist = std::uniform_int_distribution<std::mt19937::result_type>{};

    auto th_one = std::thread{[&]() {
        for (auto i = 0; i < number_iterations; ++i) {
            auto current_value = dist(rng);
            results[i] = current_value;
            c.send(current_value);
        }
    }};

    auto th_two = std::thread{[&]() {
        for (auto i = 0; i < number_iterations; ++i) {
            auto val = c.read();
            EXPECT_EQ(val, results[i]);
        }
    }};

    th_one.join();
    th_two.join();
}

TEST(Channel, SelectBasicRead) {
    sharp::Channel<int> c{1};
    c.send(1);
    int val = 0;
    sharp::channel_select(
        std::make_pair(std::ref(c), [&val](auto value) {
            ++val;
            EXPECT_EQ(value, 1);
        }),
        std::make_pair(std::ref(c), []() -> int {
            EXPECT_TRUE(false);
            return 0;
        })
    );
    EXPECT_EQ(val, 1);
}

TEST(Channel, SelectBasicWrite) {
    sharp::Channel<int> c{1};
    int val = 0;
    sharp::channel_select(
        std::make_pair(std::ref(c), [](auto) {
            EXPECT_TRUE(false);
        }),
        std::make_pair(std::ref(c), [&val]() -> int {
            ++val;
            return 2;
        })
    );
    auto value = c.try_read();
    EXPECT_TRUE(value);
    EXPECT_EQ(value.value(), 2);
}
