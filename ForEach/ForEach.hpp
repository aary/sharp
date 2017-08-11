/**
 * @file Foreach.hpp
 * @author Aaryaman Sagar
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

} // namespace sharp

#include <sharp/ForEach/ForEach.ipp>
