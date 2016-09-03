/**
 * @file Optional.hpp
 * @author Aaryaman Sagar (rmn100@gmail.com)
 *
 * This file contains an Optional data type which differs from popular
 * implementations in one important way - most implementaions of an optional
 * are similar to std::pair<Type, bool> with the bool representing whether the
 * object is valid or not.  This module allows the user to use an optional
 * type without any extra memory overhead.
 */

#pragma once

namespace sharp {

/*
 * Implementation specific things that users should not try and use
 */
namespace detail {

/**
 * A tag that implies no special null policy for the type contained in the
 * optional.  When this tag is passed in as the second template argument to
 * the Optional type the implementation selects the implementation of the
 * Optional type with an extra boolean taking up space.
 */
template <typename Type>
class NoNullPolicy{};

/**
 * The Optional base class that contains member methods for things like
 * emplacing the object into the container at runtime.  The move constructors
 * and things like that.  The Optional implementations inherit from this to
 * provide further functionality.
 *
 * This base class does not do anything that make the "Optional" special.  It
 * is just meant to take care of the placement new-ing
 */
template <typename Type>
class OptionalBase;

/**
 * Partial specialization for when the null policy is actually given.  This
 * specialization is optimized for space and uses the null policy to dictate
 * whether the object contained in the optional is null or not.  For example
 * the null policy may be something like the following
 *
 *  struct NullPolicy : public NullPolicyBase<NullPolicy> {
 *      static constexpr int NULL_VALUE{-1};
 *  }
 *
 * This dictates that the null policy for the contained integer value is that
 * when the integer is a -1 the optional should be treated such that it has a
 * null value.
 *
 * What happens in the case where the optional receives a "null" value at
 * runtime?  An exception is thrown with a strong exception guarantee to alert
 * the user of that a null value snuck in so that the user can undo his
 * illegal actions.
 *
 * To define your own null policy please read the documentation of
 * NullPolicyBase
 */
template <typename Type, typename NullPolicy>
class OptionalImpl; /* : public detail::OptionalBase<Type>; */

template <typename Type>
class OptionalImpl<Type, NoNullPolicy<Type>>;
        /* : public detail::OptionalBase<Type>; */

} // namespace detail

/**
 * The null policy base class that all user defined null policies should
 * extend from.  For example
 *
 *  struct UserDefinedNull : public NullPolicyBase<UserDefinedNull> {
 *      static constexpr int NULL_VALUE{1};
 *  }
 */
template <typename NullPolicy>
class NullPolicyBase;

/**
 * @class Optional
 *
 * A template optional data type class that is used to represent optional
 * types in a type safe way.  Either the value exists or does not exist.
 *
 * The object is not constructed into the optional until it is assigned a
 * value.  No extra memory is used on the heap.  All bookkeeping is done by
 * the type itself and on the stack
 */
template <typename Type, typename NullPolicy = detail::NoNullPolicy<Type>>
class Optional; /* : public OptionalImpl<Type, NullPolicy>; */

} // namespace sharp
