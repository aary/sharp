#pragma once

#include <sharp/Singleton/Singleton.hpp>
#include <sharp/Concurrent/Concurrent.hpp>
#include <sharp/Threads/Threads.hpp>

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
    class SingletonStorage;
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
            return this->initialized.load(/* std::memory_order_acquire */);
        }

        void initialize() override {
            // make a strong reference to the object and then assign a weak
            // reference to that object, note that thie is being done because
            // this function will be called in a threadsafe manner, i.e. there
            // will always be a lock around this function, but the weak
            // pointer can be accessed from different threads without the need
            // for synchronization, and this is because the
            // std::weak_ptr::lock() method is marked const in the standard
            // library and const methods in the C++ standard library are
            // always threadsafe
            this->object_strong = std::make_shared<ContextType>();
            this->object_weak = this->object_strong;
            this->initialized.store(true);
        }

        void destroy() override {
            // destroy the strong reference, any threads with strong
            // references are responsible for releasing said references, no
            // need to call any non-const method on the weak_ptr
            this->object_strong.reset();
            this->initialized.store(false);
        }

        /**
         * This function starts the chain of code that initializes the
         * singleton by hooking into the global singleton storage.  It should
         * only be called by the Singleton<ContextType>::get() function
         */
        void launch_creation();

    private:
        /**
         * Default constructor sets the initialized member variable to
         * false to mark that the singleton is not yet created
         */
        SingletonWrapper() : initialized{false} {}


        /**
         * An atomic boolean that has a true if the singleton has been
         * initialized and contains a false otherwise.  This is the element
         * that's used in the double checked locking interface.
         */
        std::atomic<bool> initialized;

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
         * Construct and return a pointer to the storage atomically
         */
        static SingletonStorage& get() {
            static SingletonStorage* storage = new SingletonStorage{};
            return *storage;
        }

        /**
         * only way to create a singleton storage is through the public get
         * interface
         */
        SingletonStorage(const SingletonStorage&) = delete;
        SingletonStorage(SingletonStorage&&) = delete;
        SingletonStorage& operator=(const SingletonStorage&) = delete;
        SingletonStorage& operator=(SingletonStorage&&) = delete;

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
         * A function to destroy the instances of all the singletons in the
         * right order that they were created.
         *
         * This function is static because it will be used to register in the
         * std::atexit() function call, which requires a void(*)() function
         * pointer
         */
        static void destroy_singletons();

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
                               SingletonWrapperBase*> control;

            /**
             * This contains the objects in the order that they were created, so
             * that they can be destroyed at program termination in the reverse
             * order.
             */
            std::vector<std::type_index> stack_creation;

            /**
             * A singleton cannot be created while other singletons are being
             * destroyed, signal an error if this boolean is true in the
             * function that creates a singleton
             */
            bool are_singletons_being_destroyed{false};
        };

        /**
         * The synchronized interface to the storage struct above, see
         * sharp/Concurrent
         */
        sharp::Concurrent<SingletonStorageImpl, sharp::RecursiveMutex> storage;

    };

} // namespace detail

template <typename Type>
std::shared_ptr<Type> Singleton<Type>::get_strong() {

    // get the singleton handle that stores the internal singleton instance
    // that is to be returned to the user, this is synchronized by the C++
    // runtime
    static auto& singleton_handle = detail::SingletonWrapper<Type>::get();

    // this is the first gate for the double checked locking pattern, if this
    // is successful then the singleton has already been initialized, if this
    // is not successful then the second stage of the double checked locking
    // pattern will be executed in which the lock will be acquired
    if (singleton_handle.is_initialized()) {
        return singleton_handle.get_strong();
    }

    // otherwise, its time to launch the creation, this function will have to
    // acquire the lock for double checked locking since the access above is
    // lock free
    detail::SingletonStorage::get().launch_creation<Type>();

    // then return the instance, it should be created, if not then PANIC
    assert(singleton_handle.is_initialized());
    return singleton_handle.get_strong();
}

namespace detail {

    template <typename ContextType>
    SingletonWrapper<ContextType>& SingletonWrapper<ContextType>::get() {
        static SingletonWrapper<ContextType>* self =
            new SingletonWrapper<ContextType>{};
        return *self;
    }

    template <typename ContextType>
    void SingletonStorage::launch_creation() {
        auto& singleton_storage = SingletonStorage::get();

        // acquire the lock for the storage and then construct the element,
        // the construction can push back other elements into the vector that
        // stores the order of construction
        auto lock = singleton_storage.storage.lock();

        // get the instance of a singleton wrapper for the type
        auto& wrapper_ptr = SingletonWrapper<ContextType>::get();

        // if there is a circular dependence then the object will already
        // exist in the storage, leading to a double creation of the object,
        // at that point signal the user that this is an error before double
        // creating the object
        if (lock->control.find(std::type_index{typeid(ContextType)})
                != lock->control.end()) {
            throw std::runtime_error{"Circular singleton dependence"};
        }

        // if singletons are being destoryed when this is called then throw an
        // error too
        if (lock->are_singletons_being_destroyed) {
            throw std::runtime_error{"Singletons are being destroyed, cannot "
                "create a singleton now"};
        }

        // initialize the inner datum, this will initialize a strong refernece
        // and make a weak pointer point to it, subsequent accesses to the
        // datum will all be through the weak pointer
        wrapper_ptr.initialize();

        // now insert the bookkeeping into the storage, so that the instances
        // of the singleton will be destroyed in the reverse order that they
        // have been created
        auto wrapper_base_ptr
            = static_cast<SingletonWrapperBase*>(&wrapper_ptr);
        auto pair_to_insert = std::make_pair(
                std::type_index{typeid(ContextType)}, wrapper_base_ptr);
        auto return_pair = lock->control.insert(std::move(pair_to_insert));
        assert(return_pair.second);
        lock->stack_creation.push_back(std::type_index{typeid(ContextType)});
    }

} // namespace detail

} // namespace sharp
