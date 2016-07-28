#include "sharp/LockedData/LockedData.hpp"
#include "sharp/LockedData/tests/FakeMutex.hpp"
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
        assert(current_track == this->a);
        ++current_track;
    }

    virtual void unlock() override {
        this->FakeMutex::unlock();
        assert(current_track == (this->a + 1));
        --current_track;
    }

    virtual void lock_shared() override {
        this->FakeMutex::lock_shared();
        assert(current_track == this->a);
        ++current_track;
    }

    int a;
};

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
}

int main() {
    LockedDataTests::test_unique_locked_proxy();
    LockedDataTests::test_execute_atomic_non_const();
    LockedDataTests::test_execute_atomic_const();
    LockedDataTests::test_lock();
    LockedDataTests::test_lock_const();
    LockedDataTests::test_copy_constructor();
    LockedDataTests::test_in_place_construction();
    LockedDataTests::test_assignment_operator();
    return 0;
}
