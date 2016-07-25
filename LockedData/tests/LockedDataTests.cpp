#include "LockedData.hpp"
#include "FakeMutex.hpp"
#include <cassert>
using namespace sharp;

namespace sharp {
class LockedDataTests {
public:
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
        LockedData<int, FakeMutex> locked;
        auto pointer_to_object = reinterpret_cast<intptr_t>(&locked.datum);
        assert(locked.mtx.lock_state == FakeMutex::LockState::UNLOCKED);
        {
            auto proxy = locked.lock();
            assert(locked.mtx.lock_state == FakeMutex::LockState::SHARED);
            assert(reinterpret_cast<intptr_t>(&proxy.datum)
                    == pointer_to_object);
        }
        assert(locked.mtx.lock_state == FakeMutex::LockState::UNLOCKED);
    }
};
}

int main() {
    LockedDataTests::test_execute_atomic_non_const();
    LockedDataTests::test_execute_atomic_const();
    return 0;
}
