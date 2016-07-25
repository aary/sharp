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

} // namespace sharp
