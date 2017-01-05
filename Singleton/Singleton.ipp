#include <sharp/Singleton/Singleton.hpp>
#include <sharp/LockedData/LockedData.hpp>

#include <functional>
#include <vector>
#include <unordered_map>
#include <typeinfo>
#include <cassert>
#include <memory>
#include <atomic>

namespace sharp {

namespace detail {

    /**
     * An abstract base class for singletons, instances of this class are
     * stored in the singleton storage to order creation and destruction.
     */
    class SingletonWrapperBase {
    public:

        ~SingletonWrapperBase() = default;

        /**
         * Double checked locking interface, this function is the thread safe
         * first gate that should be checked before indulging in mutecii
         */
        virtual bool is_initialized() = 0;

        /**
         * Function that creates the object and places it in the internal
         * storage of the SingletonWrapper
         */
        virtual void initialize() = 0;

        /**
         * Function that calls the destructor of the object that is stored
         * internally
         */
        virtual void destroy() = 0;
    };

    /**
     * The class that contains the instance of the singleton, this is the
     * class that offers the double checked locking interface to the singleton
     * storage and to the singleton itself.  The reason for this two level
     * locking is serialization of the singletons, i.e. the singletons used
     * by this library maintain a good ordering of dependencies with each
     * other.  A singleton x that depends on another singleton y will be
     * initialized only after y, and y will be destroyed only after x.  The
     * accesses are ordered, and this object only creates the object stored
     * when the storage tells it to do so
     *
     * The concept behind this type is that the singleton initializes one
     * version of the double checked locking pattern for each type that its
     * initialized with, since each SingletonWrapper instance initialized with
     * a different type is a different class entirely, so for instance a
     * SingletonWrapper<int> and a SingletonWrapper<double> have different
     * inner variables
     */
    template <typename ContextType>
    class SingletonWrapper : public SingletonWrapperBase {
    public:

        /**
         * A singleton itself, this object is a global meyers singleton that
         * is used to wrap around the regular singleton to provide the double
         * checked locking interface, objects of this type are therefore used
         * to check if an instance exists, create one and repeat
         */
        static SingletonWrapper& get() {
            // this line of code is double checked itself, eliminate this
            // class and provide the double checked locking pattern by making
            // this contain only
            static SingletonWrapper<ContextType> self;
            return self;
        }

        /**
         * Get the internal object
         */
        ContextType& get_object() const {
            assert(this->object);
            return *this->object.get();
        }

        bool is_initialized() override {
            return this->is_initialized.load(/* std::memory_order_acquire */);
        }

        void initialize() override {
            this->object = std::make_unique<ContextType>();
            this->is_initialized.store(true);
        }

        void destroy() override {
            this->object.reset(nullptr);
            this->is_initialized.store(false);
        }

        /**
         * This function starts the chain of code that initializes the
         * singleton by hooking into the global singleton storage.  It should
         * only be called by the Singleton<ContextType>::get() function
         */
        void launch_creation();

    private:
        /**
         * Default constructor sets the is_initialized member variable to
         * false to mark that the singleton is not yet created
         */
        SingletonWrapper() : is_initialized{false} {}


        /**
         * An atomic boolean that has a true if the singleton has been
         * initialized and contains a false otherwise.  This is the element
         * that's used in the double checked locking interface.
         */
        std::atomic<bool> is_initialized;

        /**
         * The actual datum that is stored in this object
         */
        std::unique_ptr<ContextType> object;
    };

    /**
     * The singleton class that is used by the sharp::Singleton behind the
     * scenes to store the singletons and destroy them as needed
     */
    class SingletonStorage {
    public:
        /**
         * The getter for the singleton pattern, this is lazily initialized so
         * that there is no race with the regular singletons and this
         * singleton
         */
        static SingletonStorage& get() {
            // meyers singleton
            static SingletonStorage self;
            return self;
        }

        /**
         * This destroys stuff
         */
        ~SingletonStorage();

    private:
        /**
         * An object that stores the metadata used for controlling the
         * lifetime of singletons.  Not using an std::pair because this is
         * clearer
         */
        struct SingletonStorageImpl {
            /**
             * The map containing the type_index of the type and an interface
             * to control the lifetime of a singleton
             */
            std::unordered_map<std::type_index, SingletonWrapperBase*> control;

            /**
             * This contains the objects in the order that they were created, so
             * that they can be destroyed at program termination in the reverse
             * order.
             */
            std::vector<std::type_index> stack_creation;
        };


        /**
         * The synchronized interface to the storage struct above, see
         * sharp/LockedData
         */
        LockedData<SingletonStorageImpl> storage;
    };

} // namespace detail

template <typename Type>
Type& Singleton<Type>::get() {

    // get the singleton handle that stores the internal singleton instance
    // that is to be returned to the user, this is synchronized by the C++
    // runtime
    static auto singleton_handle = SingletonWrapper<Type>::get();
    if (singleton_handle.is_initialized()) {
        return singleton_handle->get_object();
    }

    // otherwise, its time to launch the creation
    singleton_handle.launch_creation();

    // then return the instance, it should be created, if not then PANIC
    assert(singleton_handle->is_initialized());
    return singleton_handle->get_object();
}

namespace detail {
    template <typename ContextType>
    void SingletonWrapper<ContextType>::launch_creation() {
    }
} // namespace detail

} // namespace sharp
