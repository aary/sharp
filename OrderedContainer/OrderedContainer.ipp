#include <sharp/OrderedContainer/OrderedContainer.hpp>
#include <sharp/Traits/Traits.hpp>
#include <sharp/Tags/Tags.hpp>

#include <utility>
#include <type_traits>
#include <iterator>

namespace sharp {

/**
 * Private implementation functions for this module
 */
namespace detail {

    /**
     * Concept checks
     */
    /**
     * Various checks to determine the right method of finding the lower bound
     * of the container
     */
    template <typename Container>
    using EnableIfListInstantiation = std::enable_if_t<
            sharp::Instantiation_v<std::decay_t<Container>, std::list>>;

    template <typename Container, typename Value>
    using EnableIfHasLowerBoundMethod = std::enable_if_t<std::is_same<
            decltype(std::declval<Container>().lower_bound(
                        std::declval<Value>())),
            decltype(std::declval<Container>().lower_bound(
                        std::declval<Value>()))>::value>;

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

template <typename Container, typename Comparator>
template <typename Value>
auto OrderedTraits<Container, Comparator>::lower_bound(
        Container& container,
        const Comparator& comparator,
        const Value& value) {
    return detail::lower_bound_traits_impl(container, comparator, value,
            sharp::preferred_dispatch<1>);
}

template <typename Container, typename Comparator>
template <typename Iterator, typename Value>
std::pair<Iterator, bool> OrderedTraits<Container, Comparator>::insert(
        Container& container,
        Iterator iterator,
        Value&& value) {
}

template <typename Container, typename Comparator>
template <typename Value>
void OrderedContainer<Container, Comparator>::insert(Value&& value) {
}

template <typename Container, typename Comparator>
template <typename Value>
auto OrderedContainer<Container, Comparator>::find(const Value& value) const {
}

template <typename Container, typename Comparator>
auto OrderedContainer<Container, Comparator>::begin() {
}

template <typename Container, typename Comparator>
auto OrderedContainer<Container, Comparator>::begin() const {
}

template <typename Container, typename Comparator>
auto OrderedContainer<Container, Comparator>::end() {
}

template <typename Container, typename Comparator>
auto OrderedContainer<Container, Comparator>::end() const {
}

template <typename Container, typename Comparator>
Container& OrderedContainer<Container, Comparator>::get() {
    return this->container;
}

} // namespace sharp
