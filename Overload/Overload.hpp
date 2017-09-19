/**
 * @file Overload.hpp
 * @author Aaryaman Sagar
 *
 * A utility to help overloading lambda expressions
 */

#pragma once

namespace sharp {

/**
 * Overload pretty much any invocable
 *
 *      class Something {
 *      public:
 *          std::string identity(std::string in) { return in; }
 *      };
 *
 *      char foo(char ch) { return ch; }
 *
 *      auto overloaded = sharp::overload(
 *          [&](int a) { return a; },
 *          [&](double d) { return d; },
 *          foo,
 *          Something::print);
 *
 *      assert(overloaded(1) == 1);
 *      assert(overloaded(2.1) == 2.1);
 *      assert(overloaded('a') == 'a');
 *      assert(overloaded(Something{}, "string"s) == "string"s);
 *
 * This can be useful in several scenarios, can be used to implement double
 * dispatch, variant visiting, etc, for example
 *
 *      auto variant = std::variant<int, double>{1};
 *      std::visit(sharp::overload([](int) {}, [](double) {}), variant);
 *
 * This also contains the type lists of the functions it holds as a compile
 * time tuple typedef, so it can be used in various function introspection
 * methods, see Traits/detail/FunctionIntrospect.hpp for more details on how
 * the argument deduction works
 */
template <typename... Funcs>
auto overload(Funcs&&... funcs);

} // namespace sharp

#include <sharp/Overload/Overload.ipp>
