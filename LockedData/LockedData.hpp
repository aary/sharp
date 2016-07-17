/**
 * @file LockedData.hpp
 * @author Aaryaman Sagar (rmn100@gmail.com)
 *
 * This module contains a simple abstraction that halps in maintaining data that
 * is meant to be shared across different threads in a more elegant way than
 * maintaining an object along with its mutex
 *
 *  * Originally written for EECS 482 @ The University of Michigan *
 *
 * Simple critical sections
 *
 * Or
 *
 *      LockedData<std::vector> vector_locked;
 *      vector_locked.execute_atomic([&](auto& vector) {
 *          // do something special with the vector
 *      });
 *
 * Or similar to the std::weak_ptr interface
 *
 *      LockedData<std::vector> vector_locked;
 *      {
 *          auto handle = vector_locked.lock();
 *          handle->interface_one();
 *          handle->interface_two();
 *          handle->interface_three();
 *      }
 *
 * This of course should be extended to existing incorporate existing patterns
 * like monitors.  So something like the following should be supported
 *
 *      {
 *          auto handle = vector_locked.lock();
 *          while(some_condition()) {
 *              cv.wait(vector_locked.get_unique_lock());
 *          }
 *      }
 */

#pragma once

#include <utility>
#include <type_traits>
#include <mutex>

namespace sharp {

/**
 * Defining some of the common template things that are needed
 */
namespace detail {

    /**
     * @class Unqualified
     *
     * A template utility that removes all reference, const and volatile
     * qualifiers from a type
     */
    template <typename Type>
    class Unqualified {
    public:
        using type = typename std::remove_cv<
            typename std::remove_reference<Type>::type>::type;
    };

    /**
     * @alias Unqualified_t
     *
     * A templated typedef that returns an expression equivalent to the
     * following
     *
     *      typename Unqualified<Type>::type;
     */
    template <typename Type>
    using Unqualified_t = typename Unqualified<Type>::type;

} // namespace detail

template <typename Type, typename Mutex = std::mutex>
class LockedData {
public:

    /**
     * Synchronized function that accepts a function.  The requirements for a
     * function are enforced by SFINAE
     *
     * @param func A function that accepts an argument of type Type&, this
     *  object of type Type& will be the unwrapped element and not the
     *  LockedData element.
     * @return decltype(func(this->data)) returns an element of the same
     *  return type as the functor that is supplied.
     */
    template <typename Func>
    decltype(auto) execute_atomic(Func func) -> decltype(func(this->data));

    /**
     * Same as execute_atomic but without acquiring the internal lock
     *
     * @param func a function that is used to access the internal object
     * @return returns the exact object that is returned by the function when
     *  executed on the internal object
     */
    template <typename Func>
    decltype(auto) execute_unatomic(Func func) -> decltype(func(this->data));

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
     */
    UniqueLockedProxy lock();

    /**
     * A const version of the same lock above.  This helps to automate the
     * process of acquiring a read lock when the lock type provided is a
     * readers-writer lock or some form of shared lock
     */
    ConstUniqueLockedProxy lock() const;

    /**
     * The usual constructors for the class
     */
    LockedData()
        noexcept(std::is_nothrow_default_constructible<Type>::value) = default;
    LockedData(const LockedData& other)
        noexcept(std::is_nothrow_copy_constructible<Type>::value);
    LockedData(LockedData&& other)
        noexcept(std::is_nothrow_move_constructible<Type>::value);

    /**
     * This constructor is present to allow simulation of an aggregate type by
     * the LockedData object.  All arguments are forwarded to the constructor
     * of the data object.
     *
     * To use this form of construction, select dispatch with a
     * std::piecewise_construct_t object.  For example
     *
     *      LockedData<Class> locked {
     *          std::piecewise_construct, one, two, three};
     */
    template <typename... Args>
    explicit LockedData(std::piecewise_construct_t, Args&&... args)
        noexcept(Type(std::forward<Args>(args...)));

    /**
     * Copy assignment and move assignment operators
     */
    LockedData& operator=(LockedData& other)
        noexcept(LockedData{std::declval<LockedData>{}});
    LockedData& operator=(LockedData&& other);
};

} // namespace sharp
