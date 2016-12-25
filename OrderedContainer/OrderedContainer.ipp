#include <sharp/OrderedContainer/OrderedContainer.hpp>
#include <sharp/Traits/Traits.hpp>
#include <sharp/Tags/Tags.hpp>

#include <utility>
#include <type_traits>
#include <iterator>
#include <algorithm>
#include <deque>
#include <vector>
#include <list>
#include <map>
#include <set>

namespace sharp {

/**
 * Private implementation functions for this module
 */
namespace detail {

    /**
     * Concept checks
     */
    /**
     * Return a non error expression to be used in a SFINAE context when the
     * type passed is an instantiation of a std::list
     */
    template <typename Container>
    using EnableIfListInstantiation = std::enable_if_t<
            sharp::Instantiation_v<std::decay_t<Container>, std::list>>;
    /**
     * Return a non error expression to be used in a SFINAE context when the
     * type passed has a lower_bound member function
     */
    template <typename Container, typename Value>
    using EnableIfHasLowerBoundMethod = std::enable_if_t<std::is_same<
            decltype(std::declval<Container>().lower_bound(
                        std::declval<Value>())),
            decltype(std::declval<Container>().lower_bound(
                        std::declval<Value>()))>::value>;
    /**
     * Return a non error expression to be used in a SFINAE context when the
     * type passed is a sequence type container, i.e. in the set std::{deque,
     * vector, list}
     */
    template <typename Container, typename Value>
    using EnableIfIsSequenceContainer = std::enable_if_t<
            sharp::Instantiation_v<std::decay_t<Container>, std::list>
            || sharp::Instantiation_v<std::decay_t<Container>, std::deque>
            || sharp::Instantiation_v<std::decay_t<Container>, std::vector>>;
    /**
     * Return a non error expression to be used in a SFINAE context when the
     * type passed is a map type container, i.e.  in the set std::{map, set}
     */
    template <typename Container, typename Value>
    using EnableIfIsTreeContainer = std::enable_if_t<
            sharp::Instantiation_v<std::decay_t<Container>, std::map>
            || sharp::Instantiation_v<std::decay_t<Container>, std::set>>;

    /**
     * Implementation functions for finding the lower bound of a container.
     * These are the default implementations considering the containers within
     * the domain of the STL, i.e. the containers considered by the
     * specializations are in the set std::{vector, list, deque, map, set}
     */
    /**
     * Overload for the case when the container is a list instantiation
     */
    template <typename Container, typename Value
              EnableIfListInstantiation<Container>* = nullptr>
    auto lower_bound_traits_impl(Container& container,
                                 const Comparator& comparator,
                                 const Value& value,
                                 sharp::preferred_dispatch<1>) {
        return std::find_if_not(std::begin(container), std::end(container),
        [&value] (const auto& element) {
            return comparator(element, value);
        });
    }

    /**
     * Overload for the case when the container has a lower_bound method, that
     * should should therefore be used in all cases.  Since the container has
     * optimized that to the max possible
     */
    template <typename Container, typename Value,
              EnableIfHasLowerBoundMethod<Container, Value>* = nullptr>
    auto lower_bound_traits_impl(Container& container,
                                 const Comparator&,
                                 const Value& value,
                                 sharp::preferred_dispatch<1>) {
        return container.lower_bound(value);
    }

    /**
     * Default implementation that executes a binary search on the range, keep
     * in mind that the binary search is only binary in the case where the
     * iterators returned by std::begin and std::end are random access
     * iterators
     */
    template <typename Container, typename Value>
    auto lower_bound_traits_impl(Container& container,
                                 const Comparator& comparator,
                                 const Value& value,
                                 sharp::preferred_dispatch<0>) {
        return std::lower_bound(std::begin(container), std::end(container),
                value, comparator);
    }

} // namespace detail

template <typename Container>
template <typename Value, typename Comparator>
auto OrderedTraits<Container>::lower_bound(Container& container,
                                           const Comparator& comparator,
                                           const Value& value) {
    return detail::lower_bound_traits_impl(container, comparator, value,
            sharp::preferred_dispatch<1>{});
}

template <typename Container>
template <typename Value, typename Iterator>
auto OrderedTraits<Container>::insert(Container& container,
                                      Iterator iterator,
                                      Value&& value) {
    // all STL containers within the domain of this module already have this
    // method
    container.insert(iterator, std::forward<Value>(value));
}

template <typename Container, typename Comparator>
template <typename Value>
void OrderedContainer<Container, Comparator>::insert(Value&& value) {
    // get the lower bound from the lower bound function as defined in the
    // traits
    auto lower_bound_iter = OrderedTraits<Container>::lower_bound(
            this->container, this->comparator, value);

    // check if the value pointed to by the lower bound is already equal to
    // the value that is to be inserted
    if (lower_bound_iter != std::end(this->container)) {
        if (!this->comparator(value, *lower_bound_iter)
                && !this->comparator(*lower_bound_iter, value)) {
            return make_pair(lower_bound_iter, false);
        }
    }

    // then insert the value at the right location
    auto inserted_iter = OrderedTraits<Container>::insert(this->container,
            lower_bound_iter, std::forward<Value>(value));

    return std::make_pair(inserted_iter, true);
}

template <typename Container, typename Comparator>
template <typename Value>
auto OrderedContainer<Container, Comparator>::find(const Value& value) const {

    // import the function that unwraps a pair and returns the key type from
    // the pair
    using sharp::unwrap_pair;

    // get the lower bound to check if the element is there or not
    auto lower_bound_iter = OrderedTraits<Container>::lower_bound(
            this->container, this->comparator, value);

    // check if the lower bound iterator points to a value that is equal to
    // the value given
    if (lower_bound_iter != std::end(this->container)) {
        return std::end(this->container);
    }

    // now check if the value is equal, if it is then return the iterator to
    // the value
    if (!this->comparator(unwrap_pair(*lower_bound_iter), value)
            && !this->comparator(value, unwrap_pair(*lower_bound_iter))) {
        return lower_bound_iter;
    }

    return std::end(this->container);
}

template <typename Container, typename Comparator>
auto OrderedContainer<Container, Comparator>::begin() {
    return std::begin(this->container);
}

template <typename Container, typename Comparator>
auto OrderedContainer<Container, Comparator>::begin() const {
    return std::begin(this->container);
}

template <typename Container, typename Comparator>
auto OrderedContainer<Container, Comparator>::end() {
    return std::end(this->container);
}

template <typename Container, typename Comparator>
auto OrderedContainer<Container, Comparator>::end() const {
    return std::end(this->container);
}

template <typename Container, typename Comparator>
Container& OrderedContainer<Container, Comparator>::get() {
    return this->container;
}

template <typename Container, typename Comparator>
const Comparator& OrderedContainer<Container, Comparator>::get_comparator() {
    return this->comparator;
}

} // namespace sharp
