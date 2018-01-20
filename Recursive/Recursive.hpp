/**
 * @file Recursive.hpp
 * @author Aaryaman Sagar
 *
 * A utility for writing self referential lambdas
 */

#pragma once

namespace sharp {

/**
 * Recursive lambdas at 0 runtime cost
 *
 * There was a C++ paper about how to make lambdas recursive recently here
 * http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0839r0.html.  This
 * library presents an identical solution with no loss of generality with
 * almost a non existent syntax overhead.  The usage is simple
 *
 *      auto f = sharp::recursive([](auto& self, int start, int end, int sum) {
 *          if (start == end) {
 *              return sum;
 *          }
 *
 *          self(start + 1, end, sum + start);
 *      });
 *      assert(f(0, 5, 0) == 10);
 *
 * The self parameter is a shim that can be used to refer to the closure
 * itself, and nothing more.  It is similar to naming the lambda
 *
 * Lambdas wrapped with sharp::recursive can also decay to function pointers
 * if they do not capture any state.  Just as regular lambdas
 *
 *      using FPtr_t = int (*) (int, int, int);
 *      FPtr_t f = sharp::recursive([](auto& self, int start, int end, int s) {
 *          if (start == end) {
 *              return s;
 *          }
 *
 *          f(start + 1, end, s + start);
 *      });
 *      assert(f(0, 5, 0) == 10);
 *
 * But since lambdas deduce the return type of closure there are times when
 * you have to explicitly specify the return type.  This manifests only when
 * the first return statement is seen after a recursive invocation, for
 * example if the above example was rewritten like so
 *
 *      sharp::recursive([](auto& self, int start, int end, int sum) -> int {
 *          if (start != end) {
 *              self(start + 1, end, sum + 1);
 *          }
 *          return sum;
 *      });
 *
 * The lambda above needs the trailing return type as the first return
 * statement is seen after the first recursive invocation.  This works just as
 * with regular functions.  This is required to be compatible with the library
 * as it is impossible to be generic without imposing this restriction.  If
 * this were not a requirement type introspection would not work with closures
 * that result from wrapping a lambda with sharp::recursive
 *
 * This thus presents a solution which is just as good as the library solution
 * without any loss of generality
 */
template <typename Lambda>
auto recursive(Lambda&&);

} // namespace sharp

#include <sharp/Recursive/Recursive.ipp>
