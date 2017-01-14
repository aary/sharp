/*
 * @file Singleton.hpp
 * @author Aaryaman Sagar (rmn100@gmail.com)
 */

#pragma once

#include <memory>

namespace sharp {

template <typename Type>
class Singleton {
public:

    /**
     * The accessor function that can be used to create and use singleton
     * instances
     */
    static std::shared_ptr<Type> get_strong();
};

} // namespace sharp

#include <sharp/Singleton/Singleton.ipp>
