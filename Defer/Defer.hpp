/**
 * @file Defer.hpp
 * @author Aaryaman Sagar
 *
 * This file implements the DEFER macro, this allows users to create functions
 * that will execute upon function exit, similar to the native Go defer()
 */

#pragma once

#include <type_traits>
#include <utility>

namespace sharp {

/**
 * @function defer
 *
 * Returns the deferred state that should be stored somewhere in a local
 * variable, this is the idiomatic way to defer in C++, with a variable that
 * stores the function and then executes on destruction
 */
template <typename Func>
auto defer(Func&& func_in);

/**
 * @function defer_guard
 *
 * Analagous to the difference between unique_lock and lock_guard; this
 * function is an optimization over the last one in the case where you don't
 * want the ability to reset a deferred function to a no-op
 */
template <typename Func>
auto defer_guard(Func&& func_in);

} // namespace sharp

#include <sharp/Defer/Defer.ipp>
