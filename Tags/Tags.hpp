/**
 * @file Tags.hpp
 * @author Aaryaman Sagar (rmn100@gmail.com)
 *
 * This module contains simple tags that I have used to be explicit with tag
 * dispatch in some cases, for example tag dispatch can be used to fix the
 * ambiguity introdced by construction with std::initializer_list like so
 *
 *  Example example{initializer_list_contruct, {1, 2, 3, 4, 5}};
 *
 * Here the Example class can be defined like follows
 *
 *  class Example {
 *  public:
 *      Example(initializer_list_contruct_t, std::initializer_list<int>)
 *  };
 *
 * So it is clear that the initializer list constructor is being called.  This
 * sort of thing can help with template code where it is not clear what
 * sequence of elements comprise of a valid constructor call for the container
 * object.
 */

#pragma once

namespace sharp {

/**
 * @class initializer_list_contruct_t
 *
 * Tag used to disambiguate the ambiguity introduced by std::initializer_list
 * and the uniform initialization syntax.  One can think they are one and the
 * same.  Now that the syntax has been standardized and has been included in
 * three consecutive standard changes (C++11, C++14 and C++17) it seems
 * unlikely that this ambiguity is going to be fixed.  So this is a simple
 * construct that helps in disambiguating the use of std::initializer_list in
 * constructors.  This can be helpful in template code where it is not clear
 * what sequence of elements comprise of a valid constructor call for the
 * container object.
 *
 * For example
 *
 *  Example example{initializer_list_contruct, {1, 2, 3, 4, 5}};
 *
 * Here the Example class can be defined like follows
 *
 *  class Example {
 *  public:
 *      Example(initializer_list_contruct_t, std::initializer_list<int>)
 *  };
 *
 * Note that const objects have internal linkage so they do not introduce
 * linker errors when multiple cpp files include this header file.
 */
class initializer_list_contruct_t {};
const initializer_list_contruct_t initializer_list_contruct;

/**
 * @class variadic_construct_t
 *
 * Tag used to disambiguate construction with variadic arguments.  This is
 * especially useful in simple wrappers which simulate functionality of
 * aggregates.
 *
 * For example
 *
 *  template <typename Type>
 *  class SimulatedAggregate { ... };
 *
 *  SimulatedAggregate<Complex> object{variadic_construct,
 *      std::forward<Args>(args)...};
 *
 * This can be used to show that the arguments are going to be forwarded
 * straight to the inner element stored in the other class, in this case
 * SimulatedAggregate
 */
class variadic_construct_t {};
const variadic_construct_t variadic_construct;

/**
 * @class delegate_constructor_t
 *
 * A tag used for RAII based constructor decoration.  This technique can be
 * described as executing code before and after initialization of a class's
 * contained members.  Since the only methods and functions that are allowed
 * to be called before element initialization in a constructor are other
 * constructors this tag can be used to explicitly show that another
 * constructor is being used just to show that code is executing before
 * initialization
 */
class delegate_constructor_t {};
const delegate_constructor_t delegate_constructor;

/**
 * @class implementation_t
 *
 * A tag used to seperate the handle from the actual underlying
 * implementation.  This is more versatile than actually disambiguating
 * between handle and implementation by virtue of different but related names
 * primarily because it allows usage in more situations, for example template
 * specialization and constructor delegation.  For usage with constructors
 * this can be considered a part of the idiom described with
 * delegate_constructor_t
 */
class implementation_t {};
const implementation_t implementation;

} // namespace sharp
