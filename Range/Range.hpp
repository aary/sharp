/**
 * @file Range.hpp
 * @author Aaryaman Sagar
 *
 * A simple range module like the one in Python, it can be used with the range
 * based for loop syntax and has the same semantics as the one in Python
 */

#pragma once

namespace sharp {

/**
 * Implementations of the container and iterator that are used to mimic the
 * range fucntionality
 */
namespace detail {
class Range {
    int begin; int end;
};
} // namespace detail

/**
 * The range function that should be embedded in a range based for loop like
 * so
 *
 *      for (auto ele : range(0, 10)) { ... }
 *
 * This will return a proxy that can support increments on its iterators till
 * the entire range has been covered, in the optimized builds, the extra
 * inefficiencies from creating objects and iterators will likely be optimized
 * away and the result will be suprisingly similar to a hand written for loop
 */
detail::Range range(int begin, int end);
