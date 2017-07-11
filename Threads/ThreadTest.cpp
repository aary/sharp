#include <sharp/Threads/ThreadTest.hpp>

#include <mutex>
#include <condition_variable>

namespace sharp {

namespace {

    std::mutex mtx;
    std::condition_variable cv;
    int current_mark{0};

} // namespace anonymous

namespace detail {

    ThreadTestRaii::ThreadTestRaii(int value_in) : value{value_in} {
        // wait for the current mark to be set by others
        auto lck = std::unique_lock<std::mutex>{mtx};
        while(current_mark != this->value) {
            cv.wait(lck);
        }
    }

    void ThreadTestRaii::release() {
        auto lck = std::unique_lock<std::mutex>{mtx};
        ++current_mark;
        cv.notify_all();
    }

    ThreadTestRaii::~ThreadTestRaii() {
        if (this->should_release) {
            this->release();
        }
    }

    ThreadTestRaii::ThreadTestRaii(ThreadTestRaii&& other) {
        this->value = other.value;
        this->should_release = other.should_release;
        other.should_release = false;
    }
}

detail::ThreadTestRaii ThreadTest::mark(int value) {
    return detail::ThreadTestRaii{value};
}

void ThreadTest::reset() {
    std::lock_guard<std::mutex> lck{mtx};
    current_mark = 0;
    cv.notify_all();
}

} // namespace sharp
