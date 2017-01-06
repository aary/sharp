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
        static SingletonWrapper& get();

        /**
         * Friend with the storage class
         */
        friend class sharp::detail::SingletonStorage;

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
         * The actual datum that is stored in this object, this is shared
         * because weak_ptrs will be passed around that point to this object
         * to take care of thread safe deletion, otherwise what can happen is
         * that a thread can call the ::get() function and then at the same
         * time the singleton can get destructed, thus dangling the reference
         * with the client, whereas in this case calling .lock() on the
         * singleton will ensure that the singleton remains valid.  This is
         * the only way to ensure thread safe singleton access ¯\_(ツ)_/¯
         */
        std::shared_ptr<ContextType> object;
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
         * Creates an instance of a SingletonWrapper<ContextType> and then
         * stores the pointer to that in the internal unordered map, the
         * pointer can be stored because the type of the wrapper is erased
         * through the abstract base class
         */
        template <typename ContextType>
        SingletonWrapper<ContextType>* create_wrapper();

        /**
         * This destroys stuff, if while this is running, someone calls the
         * ::get<> method, this will cause an assertion to fail
         */
        ~SingletonStorage();

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
        static void launch_creation();

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
            std::unordered_map<std::type_index,
                               std::unique_ptr<SingletonWrapperBase>> control;

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
        sharp::LockedData<SingletonStorageImpl> storage;
    };

    /**
     * An indirection that is used to keep track of whether the singleton
     * storage has been destroyed or not, the only place to put this is in a
     * value that will never be destroyed, because if this is put as a member
     * variable in the SingletonStorage class then calls to
     * Singleton<Type>::get() will not be able to tell whether the destruction
     * has happened, since he storage itself has been destroyed
     *
     * A small price to pay for global serialization
     */
    class HasStorageBeenDestroyed {
    public:
        static HasStorageBeenDestroyed& get() {
            static HasStorageBeenDestroyed* self = new HasStorageBeenDestroyed;
        }

        /**
         * The variable that determines whether things have been destroyed.
         */
        std::atomic<bool> has_been_destroyed;

    private:
        HasStorageBeenDestroyed() : has_been_destroyed{false} {}
    };

} // namespace detail

template <typename Type>
std::weak_ptr<Type> Singleton<Type>::get() {

    // fail an assertion if the storage has been destroyed
    assert(!HasStorageBeenDestroyed::get().has_been_destroyed.load());

    // get the singleton handle that stores the internal singleton instance
    // that is to be returned to the user, this is synchronized by the C++
    // runtime
    static auto& singleton_handle = SingletonWrapper<Type>::get();
    if (singleton_handle.is_initialized()) {
        return singleton_handle->get_object();
    }

    // otherwise, its time to launch the creation, this function will have to
    // acquire the lock for double checked locking in two places, one for the
    // singleton_handle above and one for the HasStorageBeenDestroyed boolean,
    // both can be gated with the same mutex in the storage
    SingletonStorage::launch_creation<Type>();

    // then return the instance, it should be created, if not then PANIC
    assert(singleton_handle->is_initialized());
    return singleton_handle->get_object();
}

namespace detail {

    template <typename ContextType>
    SingletonWrapper<ContextType>* SingletonStorage::create_wrapper() {

        // construct the unique_ptr to return and then store the value of that
        // in another variable to return
        auto wrapper_base = std::unique_ptr<SingletonWrapperBase>{
            std::make_unique<SingletonWrapper<ContextType>>()}
        auto value_to_return = static_cast<SingletonWrapper<ContextType>*>(
                wrapper_base.get());

        // construct the pair to insert
        auto pair_to_insert =
            std::make_pair(std::type_index{typeid(ContextType)},
                    std::move(wrapper_base));


        // add it to the internal unordered_map, this is atomic and guarded by
        // the mutex in the item this->storage
        auto locked_storage = this->storage.lock();
        locked_storage->control.insert(std::move(pair_to_insert));
        locked_storage->stack_creation.push_back(
                std::type_index{typeid(ContextType)});

        return value_to_return;
    }

    template <typename ContextType>
    SingletonWrapper<ContextType>& SingletonWrapper<ContextType>::get() {
        // this line of code registers the wrapper with the storage and then
        // returns a reference to it, since the variable is static
        // initialization happens once and only once and is made atomic by the
        // C++ runtime
        //
        // TODO remove the mutex from the locked_storage function call for
        // this, since the C++ runtime makes this atomic
        static SingletonWrapper<ContextType>* self =
            SingletonStorage::create_wrapper<ContextType>();
        return *self;
    }

    template <typename ContextType>
    void SingletonStorage::launch_creation() {
        // try and get the Singleton Storage
    }

} // namespace detail

} // namespace sharp
