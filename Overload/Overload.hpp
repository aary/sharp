/**
 * @file Overload.hpp
 * @author Aaryaman Sagar
 *
 * A utility to help overloading lambda expressions
 */

#pragma once

namespace sharp {

/**
 * Overload lambda expressions into one functor, example:
 *
 *  auto overloaded = sharp::make_overload(
 *      [&](int a) { return a; },
 *      [&](double d) { return d; });
 *
 *  assert(overloaded(1) == 1);
 *  assert(overloaded(2.1) == 2.1);
 *
 * This can be useful in several scenarios, can be used to implement double
 * dispatch, variant visiting, etc, for example
 *
 *  auto variant = std::variant<int, double>{1};
 *  std::visit(sharp::make_overload([](int) {}, [](double) {}), variant);
 *
 * In C++17 the implementation of make_overload is trivial because of new
 * aggregate type rules
 *
 *  template <typename... Funcs>
 *  class Overload : public Funcs... {
 *  public:
 *       using Funcs::operator()...;
 *  };
 *  template <typename... Funcs>
 *  Overload(Funcs&&...) -> Overload<std::decay_t<Funcs>...>;
 *
 * and then can be used directly with template constructor deduction rules
 *
 *  auto overloaded = Overload{
 *      [](int a) { return a; },
 *      [](double d) { return d; }};
 *
 *  assert(overloaded(1) == 1);
 *  assert(overloaded(2.1) == 2.1);
 *
 * But in the current versions of the compiler and even for clang in C++17
 * the above does not work.  So this version has been provided as a workaround
 */
template <typename... Funcs>
auto make_overload(Funcs&&... funcs);

} // namespace sharp

#include "Overload.ipp"
