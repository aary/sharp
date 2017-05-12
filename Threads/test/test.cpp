#include <thread>
#include <mutex>
#include <condition_variable>
#include <set>
#include <queue>
#include <iostream>
#include <string>
#include <chrono>

#include <sharp/Threads/Threads.hpp>
#include <sharp/Threads/ThreadTest.hpp>
#include <gtest/gtest.h>

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

TEST(ThreadTest, simple_thread_test_test) {
    for (auto i = 0; i < 100; ++i) {

        auto str = std::string{};

        sharp::ThreadTest::reset();

        auto th_one = std::thread{[&]() {
            auto mark = sharp::ThreadTest::mark(1);
            str += "a";
        }};

        // sleep for 10 milliseconds to make sure that the above mark is hit,
        // otherwise would use the same testing suite to write this test, but
        // that would be a circular dependence, so stress test this to make
        // sure this is correct and all future tests using this that go by the
        // assumption that this is correct will not fault as long as this is
        // correct.  This leads to a straight correctness dependence which is
        // as strong as the correctness of this test itself, which leads to a
        // much better testing correctness result.
        //
        // Test the small things the bad way and test the big things the good
        // way
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        auto mark = sharp::ThreadTest::mark(0);
        str += "b";
        mark.release();

        th_one.join();
        EXPECT_EQ(str, "ba");
    }
}
