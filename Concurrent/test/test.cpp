#include <sharp/Concurrent/Concurrent.hpp>
#include <sharp/Tags/Tags.hpp>
#include <sharp/Utility/Utility.hpp>

#include <gtest/gtest.h>

#include <cassert>

using namespace sharp;

namespace sharp {

namespace {
const auto STRESS = 1e4;
}

class FakeMutex {
public:
    enum LockState {LOCKED, SHARED, UNLOCKED};
    FakeMutex() : lock_state{LockState::UNLOCKED} {}

    virtual void lock() {
        EXPECT_EQ(this->lock_state, LockState::UNLOCKED);
        this->lock_state = LockState::LOCKED;
        static_state = LockState::LOCKED;
    }
    virtual void unlock() {
        EXPECT_EQ(this->lock_state, LockState::LOCKED);
        this->lock_state = LockState::UNLOCKED;
        static_state = LockState::UNLOCKED;
    }
    virtual void lock_shared() {
        EXPECT_EQ(this->lock_state, LockState::UNLOCKED);
        this->lock_state = LockState::SHARED;
        static_state = LockState::SHARED;
    }
    virtual void unlock_shared() {
        EXPECT_EQ(this->lock_state, LockState::SHARED);
        this->lock_state = LockState::UNLOCKED;
        static_state = LockState::UNLOCKED;
    }

    LockState lock_state;
    static LockState static_state;
};
FakeMutex::LockState FakeMutex::static_state{FakeMutex::LockState::UNLOCKED};


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
 * This is used by test ConcurrentTests::test_assignment_operator
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

class ConcurrentTests {
public:
    static void test_unique_locked_proxy() {
        EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::UNLOCKED);
        {
            auto concurrent = Concurrent<int, FakeMutex>{std::in_place, 1};
            auto proxy = concurrent.lock();
            EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::LOCKED);
            EXPECT_EQ(proxy.operator->(), &concurrent.datum);
            EXPECT_EQ(*proxy, 1);
            EXPECT_EQ(&(*proxy), &concurrent.datum);
            proxy.unlock();
            EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::UNLOCKED);
        }
        EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::UNLOCKED);

        // const unique locked proxy shuold read lock the lock
        EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::UNLOCKED);
        {
            auto concurrent = Concurrent<int, FakeMutex>{std::in_place, 1};
            auto proxy = sharp::as_const(concurrent).lock();
            EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::SHARED);
            EXPECT_EQ(proxy.operator->(), &concurrent.datum);
            EXPECT_EQ(*proxy, 1);
            EXPECT_EQ(&(*proxy), &concurrent.datum);
        }
        EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::UNLOCKED);

        EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::UNLOCKED);
        {
            auto concurrent = Concurrent<int, FakeMutex>{std::in_place, 1};
            auto proxy = concurrent.lock();
            EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::LOCKED);
            EXPECT_EQ(proxy.operator->(), &concurrent.datum);
            EXPECT_EQ(*proxy, 1);
            EXPECT_EQ(&(*proxy), &concurrent.datum);
            proxy.unlock();
            EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::UNLOCKED);
        }
        EXPECT_EQ(FakeMutex::static_state, FakeMutex::LockState::UNLOCKED);
    }

    static void test_synchronized_non_const() {
        Concurrent<double, FakeMutex> locked;
        EXPECT_EQ(locked.mtx.lock_state, FakeMutex::LockState::UNLOCKED);
        locked.synchronized([&](auto&) {
            EXPECT_EQ(locked.mtx.lock_state, FakeMutex::LockState::LOCKED);
        });
        EXPECT_EQ(locked.mtx.lock_state, FakeMutex::LockState::UNLOCKED);
    }

    static void test_synchronized_const() {
        Concurrent<double, FakeMutex> locked;
        [](const auto& locked) {
            EXPECT_EQ(locked.mtx.lock_state, FakeMutex::LockState::UNLOCKED);
            locked.synchronized([&](auto&) {
                EXPECT_EQ(locked.mtx.lock_state, FakeMutex::LockState::SHARED);
            });
            EXPECT_EQ(locked.mtx.lock_state, FakeMutex::LockState::UNLOCKED);
        }(locked);
    }

    static void test_lock() {
        Concurrent<int, FakeMutex> locked;
        EXPECT_EQ(locked.mtx.lock_state, FakeMutex::LockState::UNLOCKED);
        {
            auto proxy = locked.lock();
            EXPECT_EQ(locked.mtx.lock_state, FakeMutex::LockState::LOCKED);
        }
        EXPECT_EQ(locked.mtx.lock_state, FakeMutex::LockState::UNLOCKED);
    }

    static void test_lock_const() {
        const Concurrent<int, FakeMutex> locked{};
        auto pointer_to_object = reinterpret_cast<uintptr_t>(&locked.datum);
        EXPECT_EQ(locked.mtx.lock_state, FakeMutex::LockState::UNLOCKED);
        {
            auto proxy = locked.lock();
            EXPECT_EQ(locked.mtx.lock_state, FakeMutex::LockState::SHARED);
            EXPECT_EQ(reinterpret_cast<uintptr_t>(proxy.operator->()),
                    pointer_to_object);
        }
        EXPECT_EQ(locked.mtx.lock_state, FakeMutex::LockState::UNLOCKED);
    }

    static void test_copy_constructor() {
        Concurrent<int, FakeMutex> object{};
        Concurrent<int, FakeMutex> copy{object};
    }

    static void test_in_place_construction() {

        // construct a lockeddata object in place
        Concurrent<InPlace> locked_data{std::in_place, static_cast<int>(1)};
        static_cast<void>(locked_data);

        // assert that only one instance was created
        EXPECT_EQ(InPlace::instance_counter, 1);
    }

    static void test_assignment_operator() {

        using ConcurrentType = Concurrent<int, FakeMutexTrack>;
        using AlignedStorage = std::aligned_storage_t<sizeof(ConcurrentType)>;
        AlignedStorage l1;
        AlignedStorage l2;

        if (reinterpret_cast<uintptr_t>(&l1)
                < reinterpret_cast<uintptr_t>(&l2)) {
            new (&l1) ConcurrentType{};
            new (&l2) ConcurrentType{};
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
            new (&l2) ConcurrentType{};
            new (&l1) ConcurrentType{};

            current_track = 0;
            l1 = l2;
            l2 = l1;
        }

        reinterpret_cast<ConcurrentType*>(&l1)->~ConcurrentType();
        reinterpret_cast<ConcurrentType*>(&l2)->~ConcurrentType();
    }
};

} // namespace sharp

TEST(Concurrent, test_unique_locked_proxy) {
    ConcurrentTests::test_unique_locked_proxy();
}

TEST(Concurrent, test_synchronized_non_const) {
    ConcurrentTests::test_synchronized_non_const();
}

TEST(Concurrent, test_synchronized_const) {
    ConcurrentTests::test_synchronized_const();
}

TEST(Concurrent, test_lock) {
    ConcurrentTests::test_lock();
}

TEST(Concurrent, test_lock_const) {
    ConcurrentTests::test_lock_const();
}

TEST(Concurrent, test_copy_constructor) {
    ConcurrentTests::test_copy_constructor();
}

TEST(Concurrent, test_in_place_construction) {
    ConcurrentTests::test_in_place_construction();
}

TEST(Concurrent, test_assignment_operator) {
    ConcurrentTests::test_assignment_operator();
}

TEST(Concurrent, WaitBasic) {
    for (auto i = 0; i < STRESS; ++i) {
        auto concurrent = sharp::Concurrent<int>{std::in_place, 1};
        auto signal = sharp::Concurrent<bool>{std::in_place, false};

        auto th = std::thread{[&]() {
            auto lock = concurrent.lock();
            lock.wait([](auto& integer) {
                return integer == 2;
            });
            *signal.lock() = true;
        }};

        concurrent.synchronized([](auto& val) {
            ++val;
        });
        signal.lock().wait([](auto val) { return val; });

        th.join();
    }
}

TEST(Concurrent, WaitMany) {
    for (auto i = 0; i < STRESS; ++i) {
        const auto THREADS = 10;
        auto concurrent = sharp::Concurrent<bool>{false};
        auto signal = sharp::Concurrent<int>{0};

        auto threads = std::vector<std::thread>{};
        for (auto i = 0; i < THREADS; ++i) {
            threads.push_back(std::thread{[&]() {
                auto lock = concurrent.lock();
                lock.wait([](auto go) {
                    return go;
                });
                ++(*signal.lock());
            }});
        }

        concurrent.synchronized([](auto& val) {
            val = true;
        });
        signal.lock().wait([](auto val) { return val == THREADS; });
        EXPECT_EQ(*signal.lock(), THREADS);

        for (auto& th : threads) {
            th.join();
        }
    }
}

