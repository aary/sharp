#include <sharp/OrderedContainer/OrderedContainer.hpp>
#include <sharp/Traits/Traits.hpp>

#include <utility>

namespace sharp {

/**
 * Private implementation functions for this module
 */
namespace detail {
} // namespace detail

template <typename Container, typename Comparator>
template <typename Value>
auto OrderedTraits<Container, Comparator>::lower_bound(Container& container,
                                                       const Value& value) {
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
