/*
 * @file Singleton.hpp
 * @author Aaryaman Sagar (rmn100@gmail.com)
 */

#pragma once

namespace sharp {

template <typename Type>
class Singleton {

    /**
     * The accessor function that can be used to create and use singleton
     * instances
     */
    static Type& get();
};

} // namespace sharp

#include <sharp/Singleton/Singleton.ipp>
