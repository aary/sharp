/*
 * @file Assert.hpp
 * @author Aaryaman Sagar (rmn100@gmail.com)
 *
 * This file contains a simple thread safe assert function that also outputs a
 * message to stderr when an assertion fails.
 *
 * What does thread safety mean in this context?  Thread safety here means
 * that the output seen on the screen will not be jumbled.
 */

#pragma once

#include <sstream>
#include <iostream>

#define SHARP_ASSERT(condition, message) {                              \
    if (!condition) {                                                   \
        std::stringstream ss_err;                                       \
        ss_err << "Assertion failed " << __FILE__ << ":" << __LINE__    \
               << std::endl;                                            \
        ss_err << message << std::endl;                                 \
        std::cerr << ss_err.str() << std::endl;                         \
    }                                                                   \
}
