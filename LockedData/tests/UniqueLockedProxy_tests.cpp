#include "LockedData.hpp"
using namespace std;
using namespace sharp;

class FakeMutex {
public:
    enum class LockedState : int {LOCKED, SHARED, UNLOCKED};
    FakeMutex() : locked_state{LockedState::UNLOCKED} {}

    void lock() {
        this->locked_state = LockedState::LOCKED;
    }
    void unlock() {
        this->locked_state = LockedState::UNLOCKED;
    }
    void lock_shared() {
        this->locked_state = LockedState::SHARED;
    }

    LockedState locked_state;
};

int main() {
    auto fake_mutex = FakeMutex{};
    __attribute__((unused)) auto object = 1;
    assert(fake_mutex.locked_state == FakeMutex::LockedState::UNLOCKED);
    {
        LockedData<int, FakeMutex>::UniqueLockedProxy proxy{object,
            fake_mutex};
        assert(fake_mutex.locked_state == FakeMutex::LockedState::LOCKED);
        assert(proxy.operator->() == &object);
        assert(*proxy == 1);
        assert(&(*proxy) == &object);
    }
    assert(fake_mutex.locked_state == FakeMutex::LockedState::UNLOCKED);
}
