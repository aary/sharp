#include <sharp/Threads/Threads.hpp>

#include <gtest/gtest.h>

namespace sharp {

class FakeMutex {
public:
    enum LockState {LOCKED, SHARED, UNLOCKED};
    FakeMutex() : lock_state{LockState::UNLOCKED} {}

    virtual void lock() {
        EXPECT_EQ(this->lock_state, LockState::UNLOCKED);
        this->lock_state = LockState::LOCKED;
        static_state = LockState::LOCKED;
        ++number_locks;
    }
    virtual void unlock() {
        EXPECT_EQ(this->lock_state, LockState::LOCKED);
        this->lock_state = LockState::UNLOCKED;
        static_state = LockState::UNLOCKED;
    }
    virtual void lock_shared() {
        this->lock();
        this->lock_state = LockState::SHARED;
        static_state = LockState::SHARED;
    }
    virtual void unlock_shared() {
        EXPECT_EQ(this->lock_state, LockState::SHARED);
        this->lock_state = LockState::UNLOCKED;
        static_state = LockState::UNLOCKED;
    }
    virtual bool try_lock() {
        this->lock();
        return true;
    }
    template <typename Duration>
    bool try_lock_for(Duration&) {
        return this->try_lock();
    }
    template <typename Time>
    bool try_lock_until(Time&) {
        return this->try_lock();
    }

    LockState lock_state;
    static LockState static_state;
    static int number_locks;
};
FakeMutex::LockState FakeMutex::static_state{FakeMutex::LockState::UNLOCKED};
int FakeMutex::number_locks{0};

class UniqueLockTests : public ::testing::Test {
public:
    void SetUp() override {
        FakeMutex::static_state = FakeMutex::LockState::UNLOCKED;
        FakeMutex::number_locks = 0;
    }
};

TEST_F(UniqueLockTests, DefaultConstruct) {
    auto lck = sharp::UniqueLock<FakeMutex>{};
    EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::UNLOCKED);
    EXPECT_EQ(FakeMutex::number_locks, 0);
}

TEST_F(UniqueLockTests, LockConstruct) {
    auto mtx = FakeMutex{};
    auto lck = sharp::UniqueLock<FakeMutex>{mtx};
    EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::LOCKED);
    EXPECT_EQ(FakeMutex::number_locks, 1);
}

} // namespace sharp
