/*
 * @file Traits.hpp
 * @author Aaryaman Sagar (rmn100@gmail.com)
 *
 * This file contains some template metaprogramming interfaces that I
 * occasionally find useful in addition to the C++ standard library header
 * <type_traits>
 */

#pragma once

#include <sharp/Traits/detail/IsInstantiationOf.hpp>
#include <sharp/Traits/detail/IsCallable.hpp>
#include <sharp/Traits/detail/UnwrapPair.hpp>
#include <sharp/Traits/detail/IsOneOf.hpp>
#include <sharp/Traits/detail/TypeLists.hpp>
#include <sharp/Traits/detail/FunctionIntrospect.hpp>
#include <sharp/Traits/detail/Algorithm.hpp>
#include <sharp/Traits/detail/Functional.hpp>
#include <sharp/Traits/detail/Utility.hpp>
