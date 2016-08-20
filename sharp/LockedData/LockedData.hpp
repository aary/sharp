/**
 * @file LockedData.hpp
 * @author Aaryaman Sagar (rmn100@gmail.com)
 *
 * This module contains a simple abstraction that halps in maintaining data
 * that is meant to be shared across different threads in a more elegant way
 * than maintaining an object along with its mutex.
 *
 *  * Originally written for EECS 482 @ The University of Michigan *
 *
 * See the class description documentation for simple use case examples.
 */

#pragma once

#include <sharp/Tags/Tags.hpp>

#include <utility>
#include <type_traits>
#include <mutex>

namespace sharp {

/**
 * @class LockedData
 *
 * Simple critical sections
 *
 * Or
 *
 *  LockedData<std::vector> vector_locked;
 *  auto& ele = vector_locked.execute_atomic([&](auto& v) { return v[0]; });
 *
 * Or similar to the std::weak_ptr interface
 *
 *  LockedData<std::vector> vector_locked;
 *  {
 *      auto handle = vector_locked.lock();
 *      handle->interface_one();
 *      handle->interface_two();
 *      handle->interface_three();
 *  }
 *
 * This of course should be extended to existing incorporate existing patterns
 * like monitors.  So something like the following should be supported
 *
 *  {
 *      auto handle = vector_locked.lock();
 *      while(some_condition()) {
 *          cv.wait(vector_locked.get_unique_lock());
 *      }
 *  }
 *
 * This class provides for maximal performance when the program is const
 * correct.  i.e.  when the object is not meant to be written to then the
 * implementation appropriately selects the right locking methodology.
 *
 * Note that this class is not moveable.  If you want move semantics, because
 * for example you are including objects of this class in a map, then just put
 * it in a unique_ptr and put that pointer in the map.
 */
template <typename Type, typename Mutex = std::mutex>
class LockedData {
public:

    /**
     * Synchronized function that accepts a function.  The requirements for a
     * function are enforced by SFINAE.
     *
     * The return value is perfectly forwarded to the called by means of the
     * decltype(auto) return type.
     *
     * Note that the single rare case where this does not happen in the case
     * of `decltype((x))` where the `(x)` neccesitates decltype to return a
     * reference type has been avoided here in the implementation.
     *
     * @param func A function that accepts an argument of type Type&, this
     *             object of type Type& will be the unwrapped element and not
     *             the LockedData element.
     * @return returns the exact object that is returned by the function when
     *         executed on the internal object
     */
    template <typename Func>
    decltype(auto) execute_atomic(Func&&)
        /* -> decltype(std::declval<Func>(std::declval<Type>())) */;

    /* Const version of the same function that locks the object via acquiring
     * a read lock if the type of mutex passed in supports shared locking
     *
     * @param func A function that accepts a reference to an object of type
     *             Type
     * @return returns the exact object that is returned by the function when
     *         executed on the internal object
     */
    template <typename Func>
    decltype(auto) execute_atomic(Func&&) const
        /* -> decltype(std::declval<Func>()(std::declval<Type>())) */;

    /**
     * Forward declarations of lightweight proxy types that are used to
     * interact with the underlying object
     */
    class UniqueLockedProxy;
    class ConstUniqueLockedProxy;

    /**
     * Returns a proxy object that locks the inner data object on construction
     * and unlocks it on destruction.
     *
     * This should be updated to also return a proxy object that behaves as a
     * reference when the operator.() becomes standard.
     *
     * Note that this method has been overloaded on the basis of whether the
     * object is const or not.  When the LockedData object is const in context
     * then this function is called and when the object is mutable in context
     * then the version above is called
     */
    UniqueLockedProxy lock();

    /**
     * A const version of the same lock above.  This helps to automate the
     * process of acquiring a read lock when the lock type provided is a
     * readers-writer lock or some form of shared lock.
     *
     * Note that this method has been overloaded on the basis of whether the
     * object is const or not.  When the LockedData object is const in context
     * then this function is called and when the object is mutable in context
     * then the version above is called
     */
    ConstUniqueLockedProxy lock() const;

    /**
     * The usual constructors for the class.  This is set to the default
     * constructor for the class.
     */
    LockedData() = default;

    /**
     * Copy constructor.
     *
     * Note that the internal mutex is held here and therefore the
     * constructors dont have the option of being noexcept.
     *
     * Also note that this *does not* do any copying from the mutex.  The
     * mutex is left in its default constructed state.
     *
     * @param other the other locked data object that is to be copied
     */
    LockedData(const LockedData&);

    /**
     * Move constructor has been deleted because it probably should not be
     * used.  The inner mutex is not going to be moved.  Wrapping it in a
     * unique_ptr would cause loss of performance and is unacceptable here
     */
    LockedData(LockedData&&) = delete;

    /**
     * This constructor is present to allow simulation of an aggregate type by
     * the LockedData object.  All arguments are forwarded to the constructor
     * of the data object.
     *
     * To use this form of construction, select dispatch with a
     * sharp::emplace_construct_t object.  For example
     *
     *  LockedData<Class> locked {
     *      std::emplace_construct_t, one, two, three};
     *
     * Note that the internal mutex is not held here and therefore the
     * constructors have the option of being noexcept
     *
     * @param emplace_construct_t a tag that is used to clarify that the
     * arguments are going to be forwarded to the constructor of the
     * underlying object
     * @param args... arguments to forward to the constructor of the stored
     * object
     */
    template <typename... Args>
    explicit LockedData(sharp::emplace_construct_t, Args&&...args)
        noexcept(noexcept(Type(std::forward<Args>(args)...)));

    /**
     * Copy assignment operator
     *
     * Note that this is not declared noexcept because the locks have to be
     * held when assigning a locked object.
     */
    LockedData& operator=(const LockedData&) /* noexcept */;

    /**
     * Deleted the move assignment operator becauase I saw no reason to
     * include it.  This should not be moved across threads?
     */
    LockedData& operator=(LockedData&& other) = delete;

private:

    /**
     * Used in constructors to perform some action before and after the
     * construction of the datum.  In this case this is used in constructors
     * to directly initialize the datum while holding the lock before and
     * releasing it after the construction is done
     *
     * The delegate constructor tag is used for clarity
     *
     * Action represents a struct that performs some action on construction
     * and releases it on destruction
     */
    template <typename Action, typename... Args>
    explicit LockedData(sharp::delegate_constructor_t, Action, Args&&...);

    /**
     * Implementation of the copy constructor for the class
     */
    explicit LockedData(sharp::implementation_t, const LockedData&);

    /**
     * The data object that is to be locked
     */
    Type datum;

    /**
     * The internal mutex
     */
    mutable Mutex mtx;

    /* Friend for testing */
    friend class LockedDataTests;
};

} // namespace sharp

#include <sharp/LockedData/LockedData.ipp>
