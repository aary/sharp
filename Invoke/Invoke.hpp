/**
 * @file Invoke.hpp
 * @author Aaryaman Sagar
 */

#pragma once

namespace sharp {

/**
 * @function invoke
 *
 * This function allows the user to invoke any callable with arguments in any
 * order (with the relative ordering maintained)
 *
 * Further it natively supports optional types and allows the user to skip
 * passing those in to the invocation entirely.  It also supports
 * one-from-many in the form of variant types, where only one of the many list
 * of options can be passed in
 */
template <typename Func, typename... Args>
decltype(auto) invoke(Func&& func, Args&&... args);

} // namespace sharp
