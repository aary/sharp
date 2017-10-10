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
        ++number_unlocks;
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
        ++number_unlocks;
    }
    virtual bool try_lock() {
        if (this->should_lock) {
            this->lock();
            return true;
        } else {
            return false;
        }
    }
    template <typename Duration>
    bool try_lock_for(Duration&) {
        return this->try_lock();
    }
    template <typename Time>
    bool try_lock_until(Time&) {
        return this->try_lock();
    }

    bool should_lock{true};
    LockState lock_state;
    static LockState static_state;
    static int number_locks;
    static int number_unlocks;
};
FakeMutex::LockState FakeMutex::static_state{FakeMutex::LockState::UNLOCKED};
int FakeMutex::number_locks{0};
int FakeMutex::number_unlocks{0};

class UniqueLockTests : public ::testing::Test {
public:
    void SetUp() override {
        FakeMutex::static_state = FakeMutex::LockState::UNLOCKED;
        FakeMutex::number_locks = 0;
        FakeMutex::number_unlocks = 0;
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
    EXPECT_EQ(mtx.lock_state, FakeMutex::LockState::LOCKED);
    EXPECT_EQ(lck.owns_lock(), true);
    EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::LOCKED);
    EXPECT_EQ(FakeMutex::number_locks, 1);
    EXPECT_EQ(FakeMutex::number_unlocks, 0);
}

TEST_F(UniqueLockTests, LockMoveConstruct) {
    auto mtx = FakeMutex{};
    auto one = sharp::UniqueLock<FakeMutex>{mtx};
    EXPECT_EQ(one.owns_lock(), true);
    auto two = std::move(one);
    EXPECT_EQ(one.owns_lock(), false);
    EXPECT_EQ(two.owns_lock(), true);
    EXPECT_EQ(mtx.lock_state, FakeMutex::LockState::LOCKED);
    EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::LOCKED);
    EXPECT_EQ(FakeMutex::number_locks, 1);

    auto mtx_two = FakeMutex{};
    auto three = sharp::UniqueLock<FakeMutex>{mtx_two};
    EXPECT_EQ(three.owns_lock(), true);
    auto four = std::move(three);
    EXPECT_EQ(three.owns_lock(), false);
    EXPECT_EQ(four.owns_lock(), true);
    EXPECT_EQ(mtx_two.lock_state, FakeMutex::LockState::LOCKED);
    EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::LOCKED);
    EXPECT_EQ(FakeMutex::number_locks, 2);
}

TEST_F(UniqueLockTests, LockDefer) {
    auto mtx = FakeMutex{};
    {
        auto lck = sharp::UniqueLock<FakeMutex>{mtx, std::defer_lock};
        EXPECT_EQ(mtx.lock_state, FakeMutex::LockState::UNLOCKED);
        EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::UNLOCKED);
        EXPECT_EQ(FakeMutex::number_locks, 0);

        EXPECT_EQ(lck.owns_lock(), false);
        lck.lock();
        EXPECT_EQ(mtx.lock_state, FakeMutex::LockState::LOCKED);
        EXPECT_EQ(lck.owns_lock(), true);
        EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::LOCKED);
        EXPECT_EQ(FakeMutex::number_locks, 1);
    }

    EXPECT_EQ(mtx.lock_state, FakeMutex::LockState::UNLOCKED);
    EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::UNLOCKED);
    EXPECT_EQ(FakeMutex::number_locks, 1);
    EXPECT_EQ(FakeMutex::number_unlocks, 1);
}

TEST_F(UniqueLockTests, TryToLock) {
    auto mtx = FakeMutex{};
    {
        auto lck = sharp::UniqueLock<FakeMutex>{mtx, std::try_to_lock};
        EXPECT_EQ(lck.owns_lock(), true);
        EXPECT_EQ(mtx.lock_state, FakeMutex::LockState::LOCKED);
        EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::LOCKED);
        EXPECT_EQ(FakeMutex::number_locks, 1);
    }

    EXPECT_EQ(mtx.lock_state, FakeMutex::LockState::UNLOCKED);
    EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::UNLOCKED);
    EXPECT_EQ(FakeMutex::number_locks, 1);
    EXPECT_EQ(FakeMutex::number_unlocks, 1);

    {
        mtx.should_lock = false;
        auto lck = sharp::UniqueLock<FakeMutex>{mtx, std::try_to_lock};
        EXPECT_EQ(lck.owns_lock(), false);
        EXPECT_EQ(mtx.lock_state, FakeMutex::LockState::UNLOCKED);
        EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::UNLOCKED);
        EXPECT_EQ(FakeMutex::number_locks, 1);
    }

    EXPECT_EQ(mtx.lock_state, FakeMutex::LockState::UNLOCKED);
    EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::UNLOCKED);
    EXPECT_EQ(FakeMutex::number_locks, 1);
    EXPECT_EQ(FakeMutex::number_unlocks, 1);
}

TEST_F(UniqueLockTests, AdoptLock) {
    auto mtx = FakeMutex{};
    {
        auto lck = sharp::UniqueLock<FakeMutex>{mtx, std::adopt_lock};
        EXPECT_EQ(lck.owns_lock(), true);
        EXPECT_EQ(mtx.lock_state, FakeMutex::LockState::UNLOCKED);
        EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::UNLOCKED);
        EXPECT_EQ(FakeMutex::number_locks, 0);

        mtx.lock();
        EXPECT_EQ(lck.owns_lock(), true);
        EXPECT_EQ(mtx.lock_state, FakeMutex::LockState::LOCKED);
        EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::LOCKED);
        EXPECT_EQ(FakeMutex::number_locks, 1);
    }

    EXPECT_EQ(mtx.lock_state, FakeMutex::LockState::UNLOCKED);
    EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::UNLOCKED);
    EXPECT_EQ(FakeMutex::number_locks, 1);
    EXPECT_EQ(FakeMutex::number_unlocks, 1);

    {
        auto lck = sharp::UniqueLock<FakeMutex>{mtx, std::adopt_lock};
        EXPECT_EQ(lck.owns_lock(), true);
        EXPECT_EQ(mtx.lock_state, FakeMutex::LockState::UNLOCKED);
        EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::UNLOCKED);
        EXPECT_EQ(FakeMutex::number_locks, 1);

        mtx.lock();
        EXPECT_EQ(lck.owns_lock(), true);
        EXPECT_EQ(mtx.lock_state, FakeMutex::LockState::LOCKED);
        EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::LOCKED);
        EXPECT_EQ(FakeMutex::number_locks, 2);

        lck.unlock();
        EXPECT_EQ(lck.owns_lock(), false);
        EXPECT_EQ(mtx.lock_state, FakeMutex::LockState::UNLOCKED);
        EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::UNLOCKED);
        EXPECT_EQ(FakeMutex::number_locks, 2);
    }

    EXPECT_EQ(mtx.lock_state, FakeMutex::LockState::UNLOCKED);
    EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::UNLOCKED);
    EXPECT_EQ(FakeMutex::number_locks, 2);
    EXPECT_EQ(FakeMutex::number_unlocks, 2);
}

TEST_F(UniqueLockTests, AcquireLock) {
    auto mtx = FakeMutex{};
    {
        auto lck = sharp::UniqueLock<FakeMutex>{mtx, [](auto& m){ m.lock(); }};
        EXPECT_EQ(lck.owns_lock(), true);
        EXPECT_EQ(mtx.lock_state, FakeMutex::LockState::LOCKED);
        EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::LOCKED);
        EXPECT_EQ(FakeMutex::number_locks, 1);

        lck.unlock();
        EXPECT_EQ(lck.owns_lock(), false);
        EXPECT_EQ(mtx.lock_state, FakeMutex::LockState::UNLOCKED);
        EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::UNLOCKED);
        EXPECT_EQ(FakeMutex::number_locks, 1);
    }

    EXPECT_EQ(mtx.lock_state, FakeMutex::LockState::UNLOCKED);
    EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::UNLOCKED);
    EXPECT_EQ(FakeMutex::number_locks, 1);
    EXPECT_EQ(FakeMutex::number_unlocks, 1);

    {
        auto lck = sharp::UniqueLock<FakeMutex>{mtx, [](auto& m) {
            return m.try_lock();
        }};
        EXPECT_EQ(lck.owns_lock(), true);
        EXPECT_EQ(mtx.lock_state, FakeMutex::LockState::LOCKED);
        EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::LOCKED);
        EXPECT_EQ(FakeMutex::number_locks, 2);

        lck.unlock();
        EXPECT_EQ(lck.owns_lock(), false);
        EXPECT_EQ(mtx.lock_state, FakeMutex::LockState::UNLOCKED);
        EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::UNLOCKED);
        EXPECT_EQ(FakeMutex::number_locks, 2);
    }

    EXPECT_EQ(mtx.lock_state, FakeMutex::LockState::UNLOCKED);
    EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::UNLOCKED);
    EXPECT_EQ(FakeMutex::number_locks, 2);
    EXPECT_EQ(FakeMutex::number_unlocks, 2);

    {
        mtx.should_lock = false;
        auto lck = sharp::UniqueLock<FakeMutex>{mtx, [](auto& m) {
            return m.try_lock();
        }};
        EXPECT_EQ(lck.owns_lock(), false);
        EXPECT_EQ(mtx.lock_state, FakeMutex::LockState::UNLOCKED);
        EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::UNLOCKED);
        EXPECT_EQ(FakeMutex::number_locks, 2);
    }

    EXPECT_EQ(mtx.lock_state, FakeMutex::LockState::UNLOCKED);
    EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::UNLOCKED);
    EXPECT_EQ(FakeMutex::number_locks, 2);
    EXPECT_EQ(FakeMutex::number_unlocks, 2);
}

TEST_F(UniqueLockTests, AssignmentOperator) {
    auto mtx = FakeMutex{};

    {
        auto lck = sharp::UniqueLock<FakeMutex>{mtx};
        EXPECT_EQ(lck.owns_lock(), true);
        EXPECT_EQ(mtx.lock_state, FakeMutex::LockState::LOCKED);
        EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::LOCKED);
        EXPECT_EQ(FakeMutex::number_locks, 1);

        auto two = sharp::UniqueLock<FakeMutex>{};
        two = std::move(lck);
        EXPECT_EQ(lck.owns_lock(), false);
        EXPECT_EQ(two.owns_lock(), true);
        EXPECT_EQ(mtx.lock_state, FakeMutex::LockState::LOCKED);
        EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::LOCKED);
        EXPECT_EQ(FakeMutex::number_locks, 1);
    }

    EXPECT_EQ(mtx.lock_state, FakeMutex::LockState::UNLOCKED);
    EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::UNLOCKED);
    EXPECT_EQ(FakeMutex::number_locks, 1);
    EXPECT_EQ(FakeMutex::number_unlocks, 1);

    {
        auto lck = sharp::UniqueLock<FakeMutex>{};
        auto two = std::move(lck);
        EXPECT_EQ(lck.owns_lock(), false);
        EXPECT_EQ(two.owns_lock(), false);
        EXPECT_EQ(mtx.lock_state, FakeMutex::LockState::UNLOCKED);
        EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::UNLOCKED);
        EXPECT_EQ(FakeMutex::number_locks, 1);
    }
    EXPECT_EQ(mtx.lock_state, FakeMutex::LockState::UNLOCKED);
    EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::UNLOCKED);
    EXPECT_EQ(FakeMutex::number_locks, 1);
    EXPECT_EQ(FakeMutex::number_unlocks, 1);
}

TEST_F(UniqueLockTests, Lock) {
    auto mtx = FakeMutex{};

    {
        auto lck = sharp::UniqueLock<FakeMutex>{mtx, std::defer_lock};
        EXPECT_EQ(lck.owns_lock(), false);
        EXPECT_EQ(mtx.lock_state, FakeMutex::LockState::UNLOCKED);
        EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::UNLOCKED);
        EXPECT_EQ(FakeMutex::number_locks, 0);

        lck.lock([](auto& mtx) { mtx.lock(); });
        EXPECT_EQ(lck.owns_lock(), true);
        EXPECT_EQ(mtx.lock_state, FakeMutex::LockState::LOCKED);
        EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::LOCKED);
        EXPECT_EQ(FakeMutex::number_locks, 1);
    }

    EXPECT_EQ(mtx.lock_state, FakeMutex::LockState::UNLOCKED);
    EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::UNLOCKED);
    EXPECT_EQ(FakeMutex::number_locks, 1);
    EXPECT_EQ(FakeMutex::number_unlocks, 1);

    {
        auto lck = sharp::UniqueLock<FakeMutex>{mtx, std::defer_lock};
        EXPECT_EQ(lck.owns_lock(), false);
        EXPECT_EQ(mtx.lock_state, FakeMutex::LockState::UNLOCKED);
        EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::UNLOCKED);
        EXPECT_EQ(FakeMutex::number_locks, 1);

        mtx.should_lock = false;
        lck.lock([](auto& mtx) { return mtx.try_lock(); });
        EXPECT_EQ(lck.owns_lock(), false);
        EXPECT_EQ(mtx.lock_state, FakeMutex::LockState::UNLOCKED);
        EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::UNLOCKED);
        EXPECT_EQ(FakeMutex::number_locks, 1);
    }

    EXPECT_EQ(mtx.lock_state, FakeMutex::LockState::UNLOCKED);
    EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::UNLOCKED);
    EXPECT_EQ(FakeMutex::number_locks, 1);
    EXPECT_EQ(FakeMutex::number_unlocks, 1);

    {
        auto lck = sharp::UniqueLock<FakeMutex>{mtx, std::defer_lock};
        EXPECT_EQ(lck.owns_lock(), false);
        EXPECT_EQ(mtx.lock_state, FakeMutex::LockState::UNLOCKED);
        EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::UNLOCKED);
        EXPECT_EQ(FakeMutex::number_locks, 1);

        mtx.should_lock = true;
        lck.lock([](auto& mtx) { return mtx.try_lock(); });
        EXPECT_EQ(lck.owns_lock(), true);
        EXPECT_EQ(mtx.lock_state, FakeMutex::LockState::LOCKED);
        EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::LOCKED);
        EXPECT_EQ(FakeMutex::number_locks, 2);
    }

    EXPECT_EQ(mtx.lock_state, FakeMutex::LockState::UNLOCKED);
    EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::UNLOCKED);
    EXPECT_EQ(FakeMutex::number_locks, 2);
    EXPECT_EQ(FakeMutex::number_unlocks, 2);
}

TEST_F(UniqueLockTests, Unlock) {
    auto mtx = FakeMutex{};

    {
        auto lck = sharp::UniqueLock<FakeMutex>{mtx, std::defer_lock};
        EXPECT_EQ(lck.owns_lock(), false);
        EXPECT_EQ(mtx.lock_state, FakeMutex::LockState::UNLOCKED);
        EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::UNLOCKED);
        EXPECT_EQ(FakeMutex::number_locks, 0);

        lck.lock([](auto& mtx) { mtx.lock(); });
        EXPECT_EQ(lck.owns_lock(), true);
        EXPECT_EQ(mtx.lock_state, FakeMutex::LockState::LOCKED);
        EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::LOCKED);
        EXPECT_EQ(FakeMutex::number_locks, 1);
        EXPECT_EQ(FakeMutex::number_unlocks, 0);

        lck.unlock([](auto& mtx) { mtx.unlock(); });
        EXPECT_EQ(lck.owns_lock(), false);
        EXPECT_EQ(mtx.lock_state, FakeMutex::LockState::UNLOCKED);
        EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::UNLOCKED);
        EXPECT_EQ(FakeMutex::number_locks, 1);
        EXPECT_EQ(FakeMutex::number_unlocks, 1);
    }

    EXPECT_EQ(mtx.lock_state, FakeMutex::LockState::UNLOCKED);
    EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::UNLOCKED);
    EXPECT_EQ(FakeMutex::number_locks, 1);
    EXPECT_EQ(FakeMutex::number_unlocks, 1);
}

TEST_F(UniqueLockTests, Release) {
    auto mtx = FakeMutex{};
    FakeMutex* mtx_ptr = nullptr;

    {
        auto lck = sharp::UniqueLock<FakeMutex>{mtx};
        EXPECT_EQ(lck.owns_lock(), true);
        EXPECT_EQ(mtx.lock_state, FakeMutex::LockState::LOCKED);
        EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::LOCKED);
        EXPECT_EQ(FakeMutex::number_locks, 1);
        EXPECT_EQ(FakeMutex::number_unlocks, 0);

        mtx_ptr = lck.release();
        EXPECT_EQ(mtx_ptr, &mtx);
        EXPECT_EQ(lck.owns_lock(), false);
        EXPECT_EQ(mtx.lock_state, FakeMutex::LockState::LOCKED);
        EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::LOCKED);
        EXPECT_EQ(FakeMutex::number_locks, 1);
        EXPECT_EQ(FakeMutex::number_unlocks, 0);
    }

    EXPECT_EQ(mtx.lock_state, FakeMutex::LockState::LOCKED);
    EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::LOCKED);
    EXPECT_EQ(FakeMutex::number_locks, 1);
    EXPECT_EQ(FakeMutex::number_unlocks, 0);
    mtx_ptr->unlock();
    EXPECT_EQ(mtx.lock_state, FakeMutex::LockState::UNLOCKED);
    EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::UNLOCKED);
    EXPECT_EQ(FakeMutex::number_locks, 1);
    EXPECT_EQ(FakeMutex::number_unlocks, 1);
}

TEST_F(UniqueLockTests, TestExceptions) {
    auto mtx = FakeMutex{};

    {
        auto lck = sharp::UniqueLock<FakeMutex>{};
        EXPECT_THROW(lck.lock(), std::system_error);
    }
    {
        auto lck = sharp::UniqueLock<FakeMutex>{mtx};
        EXPECT_THROW(lck.lock(), std::system_error);
    }

    {
        auto lck = sharp::UniqueLock<FakeMutex>{};
        EXPECT_THROW(lck.unlock(), std::system_error);
    }

    {
        auto lck = sharp::UniqueLock<FakeMutex>{};
        EXPECT_THROW(lck.try_lock(), std::system_error);
    }
    {
        auto lck = sharp::UniqueLock<FakeMutex>{mtx};
        EXPECT_THROW(lck.try_lock(), std::system_error);
    }
}

} // namespace sharp
