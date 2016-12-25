/**
 * @file OrderedContainer.hpp
 * @author Aaryaman Sagar
 *
 * This file contains a class that can be used to wrap around an existing
 * container class and have that expose an ordered interface to the user, for
 * example an object of type OrderedContainer<std::vector<int>, std::less<int>>
 * is an ordered container instantiated with a vector as the underlying
 * container with elements in the container ordered as determined by the
 * comparator std::less<int> in increasing order
 *
 * This is useful when you want the flexibility of changing the underlying
 * container for faster algorithmic complexity but do not want to go through
 * the trouble of rewriting any existing code.  Replacing the type of the
 * container object is all that is required, the data will still remain
 * sorted.  A use case can be for example when the distribution of the
 * underlyign data stream that is to be stored in the container is not known
 * and the data is still required by the program to be sorted, the choice of
 * which container to use can be abstracted by this interface
 *
 * A note about implementation - This module follows the convention of doing
 * all template metaprogramming on a layer above the actual implementation of
 * the required functionality so as to allow the user to customize the
 * behavior of the adaptor in the case when a underlying container is used
 * that is not in the domain of containers supported by the adaptor, if you
 * use a container that is not in the STL than please specialize the
 * OrderedTraits class for that container, in most cases this will be a
 * trivial specialization
 */

#pragma once

#include <sharp/Traits/Traits.hpp>

#include <stack>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace sharp {

/**
 * @class OrderedContainer
 *
 * A container that supports ordered ranges in its internal conatainer type,
 * any values to the container will be automatically inserted into the
 * container in an ordered fashion.
 *
 * The comparator must be provided if the container does not have a
 * comparator, else this will fail to compile as the default template
 * parameter (fetching the comparator from the container itself through the
 * key_compare typedef will not be valid and thus SFINAE will occur, and since
 * no template is found which can be correctly resolved, this will produce an
 * error
 */
template <typename Container,
          typename Comparator = typename Container::key_compare>
class OrderedContainer {
public:

    /**
     * Default constructor that accepts a comparator as input
     */
    OrderedContainer(Comparator&& comparator_in = Comparator{});

    /**
     * Static asserts to provide good error messages to the user when
     * instantiating an OrderedContainer with a type that is not valid
     */
    /**
     * Static asserts for the different types of containers that are not
     * allowed with this library
     */
    static_assert(!sharp::Instantiation_v<std::decay_t<Container>, std::stack>,
            "OrderedContainer cannot be initialized with a std::stack");
    static_assert(!sharp::Instantiation_v<std::decay_t<Container>, std::queue>,
            "OrderedContainer cannot be initialized with a std::stack");
    static_assert(!sharp::Instantiation_v<std::decay_t<Container>,
            std::unordered_map>,
            "OrderedContainer cannot be initialized with a std::stack");
    static_assert(!sharp::Instantiation_v<std::decay_t<Container>,
            std::unordered_set>,
            "OrderedContainer cannot be initialized with a std::stack");
    /**
     * Assert that the container can return iterators to the beginning and end
     * of the range with the std::begin, and std::end function calls
     */
    static_assert(std::is_same<
                decltype(std::begin(std::declval<std::decay_t<Container>>())),
                decltype(std::begin(std::declval<std::decay_t<Container>>()))>
            ::value, "The container used with OrderedContainer needs to "
                     "support fetching begin and end iterators with the "
                     "std::begin() and std::end() calls");

    /**
     * Insert the value into the container in an ordered manner as determined
     * by the comparator.
     *
     * Returns a pair with the first element being an iterator and the second
     * a boolean to show whether the value was inserted or not.  The first
     * iterator points to the element that prevented the insertion
     */
    template <typename Value>
    auto insert(Value&& value);

    /**
     * Find whether the value exists in the container and return an iterator
     * to it, if the element was not found then this should return an iterator
     * past the end of the container
     */
    template <typename Value>
    auto find(const Value& value) const;

    /**
     * Convenience iterator functions that return iterators to the beginning
     * and end of the container, these are implicitly convertible, comparable
     * and all sorts of assignable to the iterator objects returned by the
     * containers begin() and end() methods (following the right const correct
     * path)
     */
    auto begin();
    auto end();
    auto begin() const;
    auto end() const;

    /**
     * Return a reference to the internal container being stored by this class
     */
    Container& get();

    /**
     * Return a const reference to the held comparator object
     */
    const Comparator& get_comparator();

private:

    /**
     * The container and the comparator
     */
    Container container;
    Comparator comparator;
};

/**
 * A traits class that contains all the relevant code for the container that
 * is supposed to be injected into the OrderedContainer, specialize this for
 * your own container to be compatible with OrderedContainer
 *
 * All the STL containers are supported by default by the OrderedTraits class,
 * the default implementation of the traits class selects functions to execute
 * on the given range in an implementation dependent manner, so do not rely on
 * having an interaface and having the ordered traits class correctly picking
 * the right functions for maximal efficiency.  If you have a container that
 * is not an STL container then please specialize the ordered traits class
 */
template <typename Container>
class OrderedTraits {
public:
    /**
     * Used to find the lower bound for the value in the container, the
     * definition of "lower bound" is the same as the one employed by the
     * C++ standard library - lower_bound(container, element) returns an
     * iterator in the container which points to a range starting at it such
     * that the range contains elements only greater than or equal to element
     *
     * This needs to be overloaded to provide default behavior for the
     * OrderedContainer class
     *
     * If the class already contains a comparator, then feel free to ignore
     * the second argument in the implementation of this function
     */
    template <typename ContainerIn, typename Value, typename Comparator>
    static auto lower_bound(ContainerIn& container,
                            const Comparator& comparator,
                            const Value& value);

    /**
     * Used to insert the value into the container at the location specified
     * by the iterator, the value should be inserted into the container right
     * before the element pointed to by the iterator
     *
     * It should return an iterator to the inserted element
     */
    template <typename ContainerIn, typename Iterator, typename Value>
    static auto insert(ContainerIn& container,
                       Iterator iterator,
                       Value&& value);
};

} // namespace sharp

#include <sharp/OrderedContainer/OrderedContainer.ipp>
