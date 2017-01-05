#include <sharp/Singleton/Singleton.hpp>

#include <functional>
#include <vector>
#include <unordered_map>
#include <typeinfo>
#include <cassert>
#include <memory>

namespace sharp {

namespace detail {

    /**
     * A base class that is meant to erase the type of the thing that is
     * stored in derived classes.  Templated derived classes can then call
     * the methods in this base class without having to program any logic
     * about what type is actually stored in the derived class
     */
    class BaseStorage {
    public:

        /**
         * A virtual destructor so that derived classes can take care of
         * destroying the stored object.
         */
        virtual ~BaseStorage() = default;

        /**
         * A getter that returns the raw pointer that is pointed to by base
         * classes
         */
        virtual void* get() const = 0;

        /**
         * Friend the std::hash function and the operator== function so that
         * the two can be used to provide a hash table with objects of this
         * class as the key, return the hashed value of the stored type index
         * in the specialization.
         */
        friend class BaseStorageHash;
        friend bool operator==(const BaseStorage&, const BaseStorage&);

    protected:
        /**
         * A constructor that constructs the BaseStorage object with a
         * std::type_index object that is to be stored to make this a keyable
         * object with associative containers such as std::unordered_map
         *
         * Note that doing so also makes this class an abstract base class
         */
        BaseStorage() = default;
    };

    /**
     * The class who's instances are actually created and then are derived from
     * the class above.  The instances of this type are movable and are the
     * ones that are responsible for deleting, creating and moving the object
     * that is stored in it
     */
    template <typename ErasedType,
              typename PointerType = std::unique_ptr<ErasedType>>
    class BaseStorageConcrete : public BaseStorage {
    public:
        /**
         * function typedefs for the type of creator and destroyer function
         * this class expects in its constructor.  These functions do not need
         * to implement any sort of synchronization.  That is the
         * responsibility of the SingletonStorage class.
         */
        using CreateFunc = ErasedType* (*) ();
        using DestroyFunc = void (*) (ErasedType*);

        /**
         * Constructor that expects two function pointers, one to create the
         * object being stored in the class and another to destroy the same
         * object
         */
        BaseStorageConcrete(CreateFunc create, DestroyFunc destroy_in)
                :  object{create()}, destroy{destroy_in} {}

        /**
         * Override the destructor to call delete on the underlying object
         */
        ~BaseStorageConcrete() override {
            // call delete on the stored pointer, thats it
            assert(this->object);
            destroy(this->object);
        }

        void* get() const override {
            return this->object;
        }

    private:

        /**
         * The actual object
         */
        PointerType object;

        /**
         * The creator and destroying functions
         */
        DestroyFunc destroy;
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
        static SingletonStorage& get();

    private:

        /**
         * The map that contains a mapping from the type of the object to the
         * index at which it is stored in the vector.  This is done because
         * vectors reallocate memory dynamically and
         * iterators/pointers/referecnes are invalidated when resizing occurs
         */
        std::unordered_map<std::type_index, BaseStorage> storage_map;

        /**
         * This contains the objects in the order that they were created, so
         * that they can be destroyed at program termination in the reverse
         * order.
         */
        std::vector<std::type_index> stack_creation;
    };
} // namespace detail

template <typename Type>
Type& Singleton<Type>::get() {
}
