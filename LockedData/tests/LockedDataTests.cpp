#include "LockedData.hpp"
#include "FakeMutex.hpp"
#include "sharp/Tags/Tags.hpp"
#include <cassert>
using namespace sharp;

namespace sharp {

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

class LockedDataTests {
public:
    static void test_unique_locked_proxy() {
        auto fake_mutex = FakeMutex{};
        auto object = 1;
        assert(fake_mutex.lock_state == FakeMutex::LockState::UNLOCKED);
        {
            auto proxy = LockedData<int, FakeMutex>::UniqueLockedProxy{object,
                fake_mutex};
            assert(fake_mutex.lock_state == FakeMutex::LockState::LOCKED);
            assert(proxy.operator->() == &object);
            assert(*proxy == 1);
            assert(&(*proxy) == &object);
        }
        assert(fake_mutex.lock_state == FakeMutex::LockState::UNLOCKED);

        // const unique locked proxy shuold read lock the lock
        assert(fake_mutex.lock_state == FakeMutex::LockState::UNLOCKED);
        {
            auto proxy = LockedData<int, FakeMutex>::ConstUniqueLockedProxy{
                object, fake_mutex};
            assert(fake_mutex.lock_state == FakeMutex::LockState::SHARED);
            assert(proxy.operator->() == &object);
            assert(*proxy == 1);
            assert(&(*proxy) == &object);
        }
        assert(fake_mutex.lock_state == FakeMutex::LockState::UNLOCKED);
    }

    static void test_execute_atomic_non_const() {
        LockedData<double, FakeMutex> locked;
        assert(locked.mtx.lock_state == FakeMutex::LockState::UNLOCKED);
        locked.execute_atomic([&](auto&) {
            assert(locked.mtx.lock_state == FakeMutex::LockState::LOCKED);
        });
        assert(locked.mtx.lock_state == FakeMutex::LockState::UNLOCKED);
    }

    static void test_execute_atomic_const() {
        LockedData<double, FakeMutex> locked;
        [](const auto& locked) {
            assert(locked.mtx.lock_state ==
                    FakeMutex::LockState::UNLOCKED);
            locked.execute_atomic([&](auto&) {
                assert(locked.mtx.lock_state ==
                    FakeMutex::LockState::SHARED);
            });
            assert(locked.mtx.lock_state ==
                    FakeMutex::LockState::UNLOCKED);
        }(locked);
    }

    static void test_lock() {
        LockedData<int, FakeMutex> locked;
        assert(locked.mtx.lock_state == FakeMutex::LockState::UNLOCKED);
        {
            auto proxy = locked.lock();
            assert(locked.mtx.lock_state == FakeMutex::LockState::LOCKED);
        }
        assert(locked.mtx.lock_state == FakeMutex::LockState::UNLOCKED);
    }

    static void test_lock_const() {
        const LockedData<int, FakeMutex> locked{};
        auto pointer_to_object = reinterpret_cast<uintptr_t>(&locked.datum);
        assert(locked.mtx.lock_state == FakeMutex::LockState::UNLOCKED);
        {
            auto proxy = locked.lock();
            assert(locked.mtx.lock_state == FakeMutex::LockState::SHARED);
            assert(reinterpret_cast<uintptr_t>(&proxy.datum)
                    == pointer_to_object);
        }
        assert(locked.mtx.lock_state == FakeMutex::LockState::UNLOCKED);
    }

    static void test_copy_constructor() {
        LockedData<int, FakeMutex> object{};
        LockedData<int, FakeMutex> copy{object};
    }

    static void test_in_place_construction() {

        // construct a lockeddata object in place
        __attribute__((unused)) LockedData<InPlace> locked_data{
            sharp::variadic_construct, static_cast<int>(1)};

        // assert that only one instance was created
        assert(InPlace::instance_counter == 1);
    }
};
}

int main() {
    LockedDataTests::test_unique_locked_proxy();
    LockedDataTests::test_execute_atomic_non_const();
    LockedDataTests::test_execute_atomic_const();
    LockedDataTests::test_lock();
    LockedDataTests::test_lock_const();
    LockedDataTests::test_copy_constructor();
    LockedDataTests::test_in_place_construction();
    return 0;
}
