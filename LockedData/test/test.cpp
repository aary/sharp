#include <sharp/LockedData/LockedData.hpp>
#include <sharp/Tags/Tags.hpp>

#include <gtest/gtest.h>

#include <cassert>

using namespace sharp;

namespace sharp {

class FakeMutex {
public:
    enum class LockState : int {LOCKED, SHARED, UNLOCKED};
    FakeMutex() : lock_state{LockState::UNLOCKED} {}

    virtual void lock() {
        EXPECT_EQ(this->lock_state, LockState::UNLOCKED);
        this->lock_state = LockState::LOCKED;
    }
    virtual void unlock() {
        EXPECT_EQ(this->lock_state, LockState::LOCKED);
        this->lock_state = LockState::UNLOCKED;
    }
    virtual void lock_shared() {
        EXPECT_EQ(this->lock_state, LockState::UNLOCKED);
        this->lock_state = LockState::SHARED;
    }
    virtual void unlock_shared() {
        EXPECT_EQ(this->lock_state, LockState::SHARED);
        this->lock_state = LockState::UNLOCKED;
    }

    LockState lock_state;
};


// declare a class that counts the number of times it has been
// instantiated
class InPlace {
public:
    static int instance_counter;
    explicit InPlace(int) {
        ++InPlace::instance_counter;
    }
};
int InPlace::instance_counter = 0;


/**
 * This mutex class ensures that the mutex that was initialized first (another
 * invariant being that the one initialized first is also before in memory)
 * gets locked first when the assignment operator is called
 *
 * This is used by test LockedDataTests::test_assignment_operator
 */
static int current_track{0};
class FakeMutexTrack : public FakeMutex {
public:
    FakeMutexTrack() : FakeMutex{} {
        this->a = current_track++;
    }

    virtual void lock() override {
        this->FakeMutex::lock();
        EXPECT_EQ(current_track, this->a);
        ++current_track;
    }

    virtual void unlock() override {
        this->FakeMutex::unlock();
        EXPECT_EQ(current_track, (this->a + 1));
        --current_track;
    }

    virtual void lock_shared() override {
        this->FakeMutex::lock_shared();
        EXPECT_EQ(current_track, this->a);
        ++current_track;
    }

    virtual void unlock_shared() override {
        this->FakeMutex::unlock_shared();
        EXPECT_EQ(current_track, (this->a + 1));
        --current_track;
    }

    int a;
};

class LockedDataTests {
public:
    static void test_unique_locked_proxy() {
        auto fake_mutex = FakeMutex{};
        auto object = 1;
        EXPECT_EQ(fake_mutex.lock_state, FakeMutex::LockState::UNLOCKED);
        {
            auto proxy = LockedData<int, FakeMutex>::UniqueLockedProxy{object,
                fake_mutex};
            EXPECT_EQ(fake_mutex.lock_state, FakeMutex::LockState::LOCKED);
            EXPECT_EQ(proxy.operator->(), &object);
            EXPECT_EQ(*proxy, 1);
            EXPECT_EQ(&(*proxy), &object);
        }
        EXPECT_EQ(fake_mutex.lock_state, FakeMutex::LockState::UNLOCKED);

        // const unique locked proxy shuold read lock the lock
        EXPECT_EQ(fake_mutex.lock_state, FakeMutex::LockState::UNLOCKED);
        {
            auto proxy = LockedData<int, FakeMutex>::ConstUniqueLockedProxy{
                object, fake_mutex};
            EXPECT_EQ(fake_mutex.lock_state, FakeMutex::LockState::SHARED);
            EXPECT_EQ(proxy.operator->(), &object);
            EXPECT_EQ(*proxy, 1);
            EXPECT_EQ(&(*proxy), &object);
        }
        EXPECT_EQ(fake_mutex.lock_state, FakeMutex::LockState::UNLOCKED);
    }

    static void test_execute_atomic_non_const() {
        LockedData<double, FakeMutex> locked;
        EXPECT_EQ(locked.mtx.lock_state, FakeMutex::LockState::UNLOCKED);
        locked.execute_atomic([&](auto&) {
            EXPECT_EQ(locked.mtx.lock_state, FakeMutex::LockState::LOCKED);
        });
        EXPECT_EQ(locked.mtx.lock_state, FakeMutex::LockState::UNLOCKED);
    }

    static void test_execute_atomic_const() {
        LockedData<double, FakeMutex> locked;
        [](const auto& locked) {
            EXPECT_EQ(locked.mtx.lock_state, FakeMutex::LockState::UNLOCKED);
            locked.execute_atomic([&](auto&) {
                EXPECT_EQ(locked.mtx.lock_state, FakeMutex::LockState::SHARED);
            });
            EXPECT_EQ(locked.mtx.lock_state, FakeMutex::LockState::UNLOCKED);
        }(locked);
    }

    static void test_lock() {
        LockedData<int, FakeMutex> locked;
        EXPECT_EQ(locked.mtx.lock_state, FakeMutex::LockState::UNLOCKED);
        {
            auto proxy = locked.lock();
            EXPECT_EQ(locked.mtx.lock_state, FakeMutex::LockState::LOCKED);
        }
        EXPECT_EQ(locked.mtx.lock_state, FakeMutex::LockState::UNLOCKED);
    }

    static void test_lock_const() {
        const LockedData<int, FakeMutex> locked{};
        auto pointer_to_object = reinterpret_cast<uintptr_t>(&locked.datum);
        EXPECT_EQ(locked.mtx.lock_state, FakeMutex::LockState::UNLOCKED);
        {
            auto proxy = locked.lock();
            EXPECT_EQ(locked.mtx.lock_state, FakeMutex::LockState::SHARED);
            EXPECT_EQ(reinterpret_cast<uintptr_t>(&proxy.datum),
                    pointer_to_object);
        }
        EXPECT_EQ(locked.mtx.lock_state, FakeMutex::LockState::UNLOCKED);
    }

    static void test_copy_constructor() {
        LockedData<int, FakeMutex> object{};
        LockedData<int, FakeMutex> copy{object};
    }

    static void test_in_place_construction() {

        // construct a lockeddata object in place
        __attribute__((unused)) LockedData<InPlace> locked_data{
            sharp::emplace_construct::tag, static_cast<int>(1)};

        // assert that only one instance was created
        EXPECT_EQ(InPlace::instance_counter, 1);
    }

    static void test_assignment_operator() {

        LockedData<int, FakeMutexTrack> l1;
        LockedData<int, FakeMutexTrack> l2;

        if (reinterpret_cast<uintptr_t>(&l1)
                < reinterpret_cast<uintptr_t>(&l2)) {
            l1 = l2;
            l2 = l1;
        }
        else {

            // this is platform dependent so in the case where declaring two
            // variables one after the other makes the first one get
            // initialized to later in memory this code will declare
            // initialize them in the reverse order.  If they were constructed
            // with l2 going first in memory then reconstruct them to get the
            // counter initialized right
            current_track = 0;
            new (&l2) decltype(l2)();
            new (&l1) decltype(l2)();

            current_track = 0;
            l1 = l2;
            l2 = l1;
        }
    }
};

} // namespace sharp

TEST(LockedData, test_unique_locked_proxy) {
    LockedDataTests::test_unique_locked_proxy();
}

TEST(LockedData, test_execute_atomic_non_const) {
    LockedDataTests::test_execute_atomic_non_const();
}

TEST(LockedData, test_execute_atomic_const) {
    LockedDataTests::test_execute_atomic_const();
}

TEST(LockedData, test_lock) {
    LockedDataTests::test_lock();
}

TEST(LockedData, test_lock_const) {
    LockedDataTests::test_lock_const();
}

TEST(LockedData, test_copy_constructor) {
    LockedDataTests::test_copy_constructor();
}

TEST(LockedData, test_in_place_construction) {
    LockedDataTests::test_in_place_construction();
}

TEST(LockedData, test_assignment_operator) {
    LockedDataTests::test_assignment_operator();
}

