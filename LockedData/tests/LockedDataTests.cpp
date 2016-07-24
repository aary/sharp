#include "LockedData.hpp"
#include "FakeMutex.hpp"
#include <cassert>
using namespace sharp;

namespace sharp {
class LockedDataTests {
public:
    static void test_mutex_state() {
        LockedData<double, FakeMutex> locked;
        assert(locked.mtx.locked_state == FakeMutex::LockedState::UNLOCKED);
        locked.execute_atomic([&](auto&) {
            assert(locked.mtx.locked_state == FakeMutex::LockedState::LOCKED);
        });
        assert(locked.mtx.locked_state == FakeMutex::LockedState::UNLOCKED);
    }
};
}

int main() {
    LockedDataTests::test_mutex_state();
    return 0;
}
