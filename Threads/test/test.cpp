#include <sharp/Threads/Threads.hpp>
#include <gtest/gtest.h>

#include <thread>
#include <mutex>
#include <condition_variable>
#include <set>
#include <queue>
#include <iostream>

auto mark_execution_sequence_point(int);

class SequencePointImpl {
public:
    friend auto mark_execution_sequence_point(int);

    ~SequencePointImpl() {
        auto lck = std::unique_lock<std::mutex>{mtx};
        ++current_sequence;
        cv.notify_all();
    }

private:
    SequencePointImpl(int point) : point_to_wait_on{point} {
        auto lck = std::unique_lock<std::mutex>{mtx};
        while (current_sequence != this->point_to_wait_on) {
            cv.wait(lck);
        }
    }

    static std::mutex mtx;
    static std::condition_variable cv;
    static int current_sequence;

    int point_to_wait_on;
};

std::mutex SequencePointImpl::mtx;
std::condition_variable SequencePointImpl::cv;
int SequencePointImpl::current_sequence = 0;

auto mark_execution_sequence_point(int point) {
    return SequencePointImpl{point};
}

TEST(RecursiveMutex, simple_recursive_test_1) {
    sharp::RecursiveMutex recursive_mtx;
    auto th = std::thread{[&]() {
        recursive_mtx.lock();
        recursive_mtx.lock();
    }};
    th.join();
}

TEST(RecursiveMutex, simple_recursive_test_2) {
    sharp::RecursiveMutex recursive_mtx;

    auto th_one = std::thread{[&]() {
        {
            auto point = mark_execution_sequence_point(0);
            EXPECT_TRUE(recursive_mtx.try_lock());
            EXPECT_TRUE(recursive_mtx.try_lock());
            recursive_mtx.unlock();
        }

        {
            auto point = mark_execution_sequence_point(2);
            recursive_mtx.unlock();
        }
    }};

    auto th_two = std::thread{[&]() {
        {
            auto point = mark_execution_sequence_point(1);
            EXPECT_FALSE(recursive_mtx.try_lock());
        }

        {
            auto point = mark_execution_sequence_point(3);
            EXPECT_TRUE(recursive_mtx.try_lock());
        }
    }};

    th_one.join();
    th_two.join();
}

TEST(RecursiveMutex, exceptions_test_1) {
    sharp::RecursiveMutex recursive_mtx;

    // test whether unlocking the mutex multiple times, more than the limit
    // causes an exception to be thrown, it should be thrown
    try {
        recursive_mtx.lock();
        recursive_mtx.unlock();
        recursive_mtx.unlock();
        EXPECT_TRUE(false);
    } catch (...) {}

    try {
        recursive_mtx.lock();
        recursive_mtx.lock();
        recursive_mtx.unlock();
        recursive_mtx.unlock();
        recursive_mtx.unlock();
        EXPECT_TRUE(false);
    } catch (...) {}
}

TEST(RecursiveMutex, exceptions_test_2) {
    sharp::RecursiveMutex recursive_mtx;

    try {
        recursive_mtx.unlock();
        EXPECT_TRUE(false);
    } catch (...) {}

    try {
        recursive_mtx.lock();
        recursive_mtx.unlock();
        recursive_mtx.unlock();
        EXPECT_TRUE(false);
    } catch (...) {}
}
