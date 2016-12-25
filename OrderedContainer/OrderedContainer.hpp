/**
 * @file OrderedContainer.hpp
 * @author Aaryaman Sagar
 *
 * This file contains a class that can be used to wrap around an existing
 * container class and have that expose an ordered interface to the user, for
 * example an object of type OrderedContainer<std::vector<int>, std::less<int>>
 * is an ordered container instantiated with a vector as the underlying
 * container with elements in the container ordered as determined by the
 * comparator std::less<int> in increasing order.
 *
 * This is useful when you want the flexibility of changing the underlying
 * container for faster algorithmic complexity but do not want to go through
 * the trouble of rewriting any existing code.  Replacing the type of the
 * container object is all that is required, the data will still remain
 * sorted.  A use case can be for example when the distribution of the
 * underlyign data stream that is to be stored in the container is not known
 * and the data is still required by the program to be sorted, the choice of
 * which container to use can be abstracted by this interface.
 */

#pragma once

#include <sharp/Traits/Traits.hpp>

#include <stack>
#include <queue>
#include <unordered_map>
#include <utility>

namespace sharp {

namespace detail {
} // namespace detail

/**
 * @class OrderedContainer
 *
 * A container that supports ordered ranges in its internal conatainer type,
 * any values to the container will be automatically inserted into the
 * container in an ordered fashion.
 */
template <typename Container, typename Comparator = void>
class OrderedContainer {
public:

    /**
     * Static asserts for the different types of containers that are not
     * allowed with this library
     */
    static_assert(!sharp::Instantiation_v<std::decay_t<container>, std::stack>,
            "OrderedContainer cannot be initialized with a std::stack");
    static_assert(!sharp::Instantiation_v<std::decay_t<container>, std::queue>,
            "OrderedContainer cannot be initialized with a std::stack");
    static_assert(!sharp::Instantiation_v<std::decay_t<container>,
            std::unordered_map>,
            "OrderedContainer cannot be initialized with a std::stack");
    static_assert(!sharp::Instantiation_v<std::decay_t<container>,
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
     * by the comparator
     */
    template <typename Value>
    void insert(Value&& value);

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

private:
    Container container;
};

/**
 * A traits class that contains all the relevant code for the container that
 * is supposed to be injected into the OrderedContainer, specialize this for
 * your own container to be compatible with OrderedContainer
 */
template <typename Container, typename Comparator>
class OrderedTraits {

    /**
     * Used to find the lower bound for the value in the container, the
     * definition of "lower bound" is the same as the one employed by the
     * C++ standard library - lower_bound(container, element) returns an
     * iterator in the container which points to a range starting at it such
     * that the range contains elements only greater than or equal to element
     *
     * This needs to be overloaded to provide default behavior for the
     * OrderedContainer class
     */
    template <typename Value>
    static auto lower_bound(Container& container, const Value& value);

    /**
     * Used to insert the value into the container at the location in the
     * container specified by the iterator, this should be done as efficiently
     * as possible
     *
     * It should return a pair with an iterator that points to the now
     * inserted element as well a boolean that says whether the insertion was
     * successful or not
     */
    template <typename Iterator, typename Value>
    static std::pair<Iterator, bool> insert(Container& container,
                                            Iterator iterator,
                                            Value&& value);
};

} // namespace sharp
