/**
 * @file FunctionIntrospect.hpp
 * @author Aaryaman Sagar
 *
 * This file contains traits that can be used to query the declaration of a
 * function.  This would include querying the return type, the argument types
 * by number and whether an argument of a specific type is present or not in
 * the argument list.
 *
 * This works for both regular functions and any special type of functor that
 * you pass it, as long as the functor has one overload for the function,  if
 * there are multiple overloads for the same function then the type
 * introspection will not work unless you disambiguate the function with a
 * static_cast, there is no way for the trait to tell which function you are
 * talking about
 */

#pragma once


