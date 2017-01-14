#include "Singleton.hpp"

namespace sharp {
namespace detail {

    void SingletonStorage::destroy_singletons() {
        auto& singleton_storage = SingletonStorage::get();

        // acquire the lock to the storage
        auto lock = singleton_storage.storage.lock();

        // signal that singletons are being destroyed
        lock->are_singletons_being_destroyed = true;

        // loop through the objects in reverse order of creation and destroy
        // them
        for (; !lock->stack_creation.empty();) {
            auto wrapper_base_ptr = lock->control[lock->stack_creation.back()];
            wrapper_base_ptr->destroy();
            auto num_removed = lock->control.erase(lock->stack_creation.back());
            assert(num_removed == 1);
            lock->stack_creation.pop_back();
        }
    }

    SingletonStorage::SingletonStorage() {
        std::atexit(SingletonStorage::destroy_singletons);
    }

} // namespace detail
} // namespace sharp
