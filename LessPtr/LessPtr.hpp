/**
 * @file LessPtr.hpp
 * @author Aaryaman Sagar
 *
 * This module contains the implementation of a transparant comparator to
 * compare the values pointed to by any two "pointer like" types.
 *
 * For example, this can be used to order a set based on the values of the
 * things pointed to by the pointers in the set
 *
 *      std::set<std::unique_ptr<int>, LessPtr> set_ptrs;
 *      set_ptrs.insert(std::make_unique<int>(1));
 *      set_ptrs.insert(std::make_unique<int>(0));
 *
 *      // output for the following always is "0 1"
 *      for (const auto& ptr : set_ptrs) {
 *          cout << *ptr << ' ';
 *      }
 *      cout << endl;
 */

#pragma once

#include <type_traits>
#include <utility>
#include <functional>

namespace sharp {

/**
 * Implementation specific things
 */
namespace detail {

    /**
     * Concept checks
     */
    /**
     * Concept check to make sure that the two pointer types being passed to
     * the LessPtr point to types that are comparable to each other with the
     * less than operator
     */
    template <typename PointerComparableOne, typename PointerComparableTwo>
    using AreValueComparablePointerTypes = std::enable_if_t<std::is_convertible<
        decltype(*std::declval<std::decay_t<PointerComparableOne>>()
                < *std::declval<std::decay_t<PointerComparableTwo>>()),
        bool>::value>;
} // namespace detail


/**
 * @class LessPtr
 *
 * A transparent comparator to compare two pointer like types by value of the
 * things they point to
 */
class LessPtr {
public:

    /**
     * This overload contains the case when the first type of object can be
     * compared when it is on the left hand side of the comparison.  This
     * would come into play when the class for the left object has defined the
     * less than operator as a member function
     *
     * std::enable_if has been used to disable this function when the objects
     * are not dereferencible
     */
    template <typename PointerComparableOne, typename PointerComparableTwo,
              detail::AreValueComparablePointerTypes<
                  PointerComparableOne, PointerComparableTwo>* = nullptr>
    constexpr auto operator()(PointerComparableOne&& lhs,
                              PointerComparableTwo&& rhs) const {
        return *std::forward<PointerComparableOne>(lhs) <
            *std::forward<PointerComparableTwo>(rhs);
    }

    /**
     * Tell the C++ standard library template specializations that this
     * comparator is a transparent comparator
     */
    using is_transparent = std::less<void>::is_transparent;
};

} // namespace sharp
