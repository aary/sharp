/**
 * @file Foreach.hpp
 * @author Aaryaman Sagar
 *
 * This file contains a generalized iteration algorithm that gets more
 * complicated as you use more features of the iteration
 */

#pragma once

namespace sharp {

/**
 * @function for_each
 *
 * Iterate through values in a range, this works for ranges that have
 * std::get<> and std::tuple_size<> defined as well, ranges that have the
 * std::begin() and std::end() functions defined for them
 *
 * Polymorphic unary or binary lambdas can be passed in as arguments to this
 * function, the first type always has to be templated so as to accept
 * arguments of different types, the second argument is optional, but also
 * must be templated because the index at which the iteration currently is at
 * is also passed in for convenience, and this is represented as a
 * std::integral_constant<int, x> type where x is the iteration number, for
 * example
 *
 *  auto t = std::make_tuple(1, "string");
 *  sharp::for_each(t, [](auto thing, auto index) {
 *      cout << thing << " at index " << static_cast<int>(index) << endl;
 *  });
 *
 * The output for the above program would be
 *
 *  1 0
 *  string 1
 *
 * and the static_cast to int is a constexpr expression so that will be
 * can be used in constexpr situations
 */
template <typename TupleType, typename Func>
constexpr Func for_each(TupleType&& tup, Func func);

/**
 * This can be used by the user to control whether the loop breaks or not,
 * instances of loop_break or loop_continue should be returned by the function
 * passed to the for_each algorithm to determine breaking or continuing
 *
 *  sharp::for_each(range, [](auto ele, auto index) {
 *      if (index >= 3) {
 *          return loop_break;
 *      }
 *      cout << ele << endl;
 *      return loop_continue;
 *  });
 *
 * This works almost identically to break and continue in regular C++ loops,
 * except that if either continue or break is returned then the function
 * object must return an instance of either in each path
 */
namespace for_each_detail {
    enum class LoopControl : bool {
        BREAK = true,
        CONTINUE = false
    };
} // namespace for_each_detail

constexpr auto loop_break = for_each_detail::LoopControl::BREAK;
constexpr auto loop_continue = for_each_detail::LoopControl::CONTINUE;

/**
 * Utility function that can be used to get elements at a given index in a
 * range, be it compile time or runtime
 *
 * This only works if the range offers random access iterators.  Not a worry
 * though with compile time iterators
 *
 * The formal requirement for this to work is, if the range contains an
 * overloaded get<>() function (ADL defined free function or as a member
 * fucntion), then that will be used.  If that does not exist then the begin
 * iterator will be examined to see if that is a random access iterator (as
 * deemed by the presence of
 * std::iterator_traits<decltype(begin)>::iterator_category and that being the
 * same as std::random_access_iterator_tag, if that is so then the index will
 * be used to index from the starting position of the range and a reference
 * (whatever that may be qualified as based on the characteristics of the
 * iterator class, i.e. it can either be an xvalue or an lvalue) will be
 * returned to it
 */
template <typename Range, typename Index>
constexpr decltype(auto) fetch(Range&& range, Index index);

} // namespace sharp

#include <sharp/ForEach/ForEach.ipp>
