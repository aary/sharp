#include <sharp/Singleton/Singleton.hpp>
#include <sharp/LockedData/LockedData.hpp>

#include <functional>
#include <vector>
#include <unordered_map>
#include <typeinfo>
#include <typeindex>
#include <cassert>
#include <memory>
#include <atomic>
#include <utility>

namespace sharp {

namespace detail {

    /**
     * An abstract base class that offers the singleton storage a way to
     * interact with the singletons that are being stored.
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
         * Friend with the storage class
         */
        friend class sharp::detail::SingletonStorage;

        /**
         * Interface to fetch intances of the SingletonWrapper class, this
         * hooks into the singleton storage for creation and bookkeeping
         */
        static SingletonWrapper& get();

        /**
         * only way to create a singleton wrapper is through the public get
         * interface
         */
        SingletonWrapper(const SingletonWrapper&) = delete;
        SingletonWrapper(SingletonWrapper&&) = delete;
        SingletonWrapper& operator=(const SingletonWrapper&) = delete;
        SingletonWrapper& operator=(SingletonWrapper&&) = delete;

        /**
         * Get the internal object
         */
        std::shared_ptr<ContextType> get_strong() const {
            // try and acquire a strong reference to the stored object and
            // return it to the user
            return this->object_weak.lock();
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
         * The actual datum that is stored in instances of this class, at
         * construction time there is a pair of smart pointers that are
         * constructed to provide lock free thread safe access to the
         * singleton.  There are race conditions to deal with at the time of
         * cosntruction and destruction.  This library takes care of race
         * conditions at construction time with the double checked locking
         * pattern and takes care of race conditions at destruction time with
         * the smart pointers provided by the standard library.  Since the
         * std::weak_ptr::lock() method is threadsafe (it is marked const and
         * all const member functions in the standard library are thread
         * safe), multiple threads can call .lock() on the same weak_ptr at
         * the same time as one thread is destroying the main shared_ptr
         */
        std::shared_ptr<ContextType> object_strong;
        std::weak_ptr<ContextType> object_weak;
    };

    /**
     * The singleton class that is used by the sharp::Singleton behind the
     * scenes to store the singletons and destroy them as needed
     */
    class SingletonStorage {
    public:

        /**
         * Creates an instance of a SingletonWrapper<ContextType> and then
         * stores the pointer to that in the internal unordered map, the
         * pointer can be stored because the type of the wrapper is erased
         * through the abstract base class
         */
        template <typename ContextType>
        SingletonWrapper<ContextType>* create_wrapper();

        /**
         * Construct and return a pointer to the storage atomically
         */
        static SingletonStorage& get() {
            static SingletonStorage* storage = new SingletonStorage{};
            return *storage;
        }

        /**
         * Initializes the SingletonWrapper for the type passed in, the
         * wrapper is accessed after acquiring a lock on the singleton
         * storage's internal data members
         *
         * After a call to this function the appropriate SingletonWrapper for
         * the type should have an initialized member variable for the
         * singleton object
         *
         * This function can have the following "edge cases"
         *
         *  1. The SingletonStorage has already been destroyed, therefore
         *     calling this function will result in an assertion failure, as
         *     no singletons can be created after the storage has been
         *     destroyed
         *  2. It can fail when entered concurrently from two different
         *     threads, in which case one will win the race for construction
         *     of the singleton, and the other will just get the reference to
         *     the singleton back
         *
         * This function traverses the graph for singleton creation in a depth
         * first preorder manner, for example if the singleton class A depends
         * on singleton classes B and C, then the graph of dependencies which
         * looks like this
         *
         *               A
         *             /   \
         *            /     \
         *           \/     \/
         *           B       C
         *
         * The graph would be traversed as follows, {A, B, C}, this implies
         * that C can be deleted before A and B can be deleted before, so a
         * vector in the SingletonStorage class satisfies this requirement.
         */
        template <typename ContextType>
        void launch_creation();

    private:
        /**
         * the constructor for the singleton storage registers the destroy
         * singletons callback with the program exit
         */
        SingletonStorage();

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
            std::unordered_map<std::type_index,
                               std::unique_ptr<SingletonWrapperBase>> control;

            /**
             * This contains the objects in the order that they were created, so
             * that they can be destroyed at program termination in the reverse
             * order.
             */
            std::vector<std::type_index> stack_creation;

            /**
             * boolean to indicate whether the storage is already in the
             * process of creation / destruction of a singleton, in that case
             * don't reacquire the lock.  This is a workaround for a recursive
             * mutex
             */
            // bool is_creating_destroying{false};
        };

        /**
         * The synchronized interface to the storage struct above, see
         * sharp/LockedData
         */
        sharp::LockedData<SingletonStorageImpl> storage;

    };

} // namespace detail

template <typename Type>
std::shared_ptr<Type> Singleton<Type>::get_strong() {

    // get the singleton handle that stores the internal singleton instance
    // that is to be returned to the user, this is synchronized by the C++
    // runtime
    static auto& singleton_handle = SingletonWrapper<Type>::get();

    // this is the first gate for the double checked locking pattern, if this
    // is successful then the singleton has already been initialized, if this
    // is not successful then the second stage of the double checked locking
    // pattern will be executed in which the lock will be acquired
    if (singleton_handle.is_initialized()) {
        return singleton_handle->get_strong();
    }

    // otherwise, its time to launch the creation, this function will have to
    // acquire the lock for double checked locking in two places, one for the
    // singleton_handle above and one for the HasStorageBeenDestroyed boolean,
    // both can be gated with the same mutex in the storage
    SingletonStorage::get().launch_creation<Type>();

    // then return the instance, it should be created, if not then PANIC
    assert(singleton_handle->is_initialized());
    return singleton_handle->get_strong();
}

namespace detail {

    template <typename ContextType>
    SingletonWrapper<ContextType>* SingletonStorage::create_wrapper() {

        // create the singleton holder objects
        auto wrapper_ptr = new SingletonWrapper<ContextType>{};
        auto wrapper_base_ptr = static_cast<SingletonWrapperBase*>(wrapper_ptr);

        // insert the pair into the unordered_map
        auto pair_to_insert = std::make_pair(
                std::type_index{typeid(ContextType)}, wrapper_base_ptr);

        // acquire the lock and insert into storage
        auto locked_storage = this->storage.lock();
        assert(locked_storage->control.find(
                    std::type_index{typeid(ContextType)})
                != locked_storage->end());
        locked_storage->control.insert(std::move(pair_to_insert));
        locked_storage->stack_creation.push_back(
                std::type_index{typeid(ContextType)});

        return wrapper_ptr;
    }

    template <typename ContextType>
    SingletonWrapper<ContextType>& SingletonWrapper<ContextType>::get() {
        static SingletonWrapper<ContextType>* self =
            SingletonStorage::create_wrapper<ContextType>();
        return *self;
    }

    template <typename ContextType>
    void SingletonStorage::launch_creation() {
        auto& storage = SingletonStorage::get();
    }

} // namespace detail

} // namespace sharp
