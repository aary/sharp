/**
 * @file UnwrapPair.hpp
 * @author Aaryaman Sagar
 *
 * Contains a simple utility to get the "key type" value out of a container,
 * this is useful when you have a templated function operating on a container
 * but do not know if the value type of the container is a key value pair or
 * not, and if only the key is needed, then two specializations of the same
 * function are required, this utility can help reduce that need
 */

#pragma once

#include <sharp/Tags/Tags.hpp>

#include <type_traits>

namespace sharp {

namespace detail {

    /**
     * Concept check to see if a type has a first and second method
     */
    template <typename Container>
    using EnableIfIsPairType = std::enable_if_t<std::is_same<
        decltype(std::declval<std::decay_t<Container>>().first),
        decltype(std::declval<std::decay_t<Container>>().first)>::value
            && std::is_same<
        decltype(std::declval<std::decay_t<Container>>().second),
        decltype(std::declval<std::decay_t<Container>>().second)>::value>;

    /**
     * Implementation for the unwrap function, powered by the preferred
     * dispatch tag found in sharp/Tags/Tags.hpp
     */
    template <typename PairType, EnableIfIsPairType<PairType>* = nullptr>
    auto& unwrap_pair_impl(PairType& pair_object,
                           sharp::preferred_dispatch<1>) {
        return pair_object.first;
    }
    template <typename PairType>
    auto& unwrap_pair_impl(PairType& pair_object,
                           sharp::preferred_dispatch<0>) {
        return pair_object;
    }

} // namespace detail

template <typename PairType>
auto&& unwrap_pair(PairType& pair_object) {
    using sharp::preferred_dispatch;
    return detail::unwrap_pair_impl(pair_object, preferred_dispatch<1>{});
}

/**
 * Tests
 */
namespace detail { namespace test {
    template <typename First, typename Second>
    struct PairTest {
        First first;
        Second second;
    };
}} // namespace detail.test

static_assert(std::is_same<
    decltype(unwrap_pair(
            static_cast<const detail::test::PairTest<int, double>&>(
                std::declval<detail::test::PairTest<int, double>>()))),
    const int&>
        ::value, "sharp::unwrap_pair tests failed!");
static_assert(std::is_same<
    decltype(unwrap_pair(static_cast<const double&>(double{}))), const double&>
        ::value, "sharp::unwrap_pair tests failed!");
static_assert(std::is_same<
    decltype(unwrap_pair(
            static_cast<std::add_lvalue_reference_t<const char* const>>(
                std::declval<const char*>()))),
    std::add_lvalue_reference_t<const char* const>>
        ::value, "sharp::unwrap_pair tests failed!");

} // namespace sharp
